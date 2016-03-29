// https://github.com/anupak/esp8266-samplecode/tree/master/i2c_rotary_encoder
// https://github.com/anupak/esp8266-samplecode/tree/master/uart
// http://www.nxp.com/documents/data_sheet/PCF8574.pdf
// http://www.ti.com/lit/an/scpa032/scpa032.pdf

#include "osapi.h"
#include "user_interface.h"
#include "gpio.h"
#include "driver/key.h"
#include "driver/i2c_master.h"
#include "okardware.h"
#include "dht.h"
#include "network.h"

/* #undef ENABLE_OKARDWARE_PCF8574 */
#define ENABLE_OKARDWARE_PCF8574        1

#define PCF8574_I2C_ADDR        0x20
#define PCF8574_INTR_GPIO       9

// PCB level 1
#define PCF8574_PORT_CD4051_A           4
#define PCF8574_PORT_CD4051_B           5
#define PCF8574_PORT_CD4051_C           6
#define PCF8574_PORT_ROTARY_PIN1        0
#define PCF8574_PORT_ROTARY_PIN2        1
#define PCF8574_PORT_ROTARY_BUTTON      2
#define PCF8574_PORT_BUTTON             3

#define LIMIT_AMPERE_USB        650

#define ADC_AMPERE_USB          0
#define ADC_AMPERE_JACK         7
#define ADC_MOTHER_THERM        5
/* #define ADC_MSGEQ7      3 */
/* #define CD4051_PORT_EXT         6 */
// Remaining CD4051_PORT_: 0, 1, 3, 5, 6, 7

#define OKARDWARE_I2C_DIRECTION_NONE            -1
#define OKARDWARE_I2C_DIRECTION_TRANSMITTER     0
#define OKARDWARE_I2C_DIRECTION_RECEIVER        1


////////////////////////////////////////////////////////////////////////////////

/* #define KEY_NUM 1 */

/* static struct plug_saved_param plug_param; */
/* static struct keys_param keys; */
/* static struct single_key_param *single_key[KEY_NUM]; */

/* void _long_press() */
/* { */
/*   os_printf("Long press!!\n"); */
/* } */

/* void _short_press() */
/* { */
/*   os_printf("Short press!!\n"); */
/* } */

int8 pcf8574_read_mask = 0;

// direction is -1 (disabled), 0 (write), 1 (read)
static int8 pcf8574_current_direction = -1;

/**
 * Open i2c connection to PCF8574 in read or write direction
 */
bool ICACHE_FLASH_ATTR pcf8574_set_direction(int8 direction)
{
  if (direction != pcf8574_current_direction) {
    if (OKARDWARE_I2C_DIRECTION_NONE != pcf8574_current_direction ||
        OKARDWARE_I2C_DIRECTION_RECEIVER == direction) {
      i2c_master_stop();
    }
    if (OKARDWARE_I2C_DIRECTION_NONE != direction) {

      uint8 addr = (PCF8574_I2C_ADDR << 1) | direction;
      uint8 ack;

      i2c_master_start();
      i2c_master_writeByte(addr);
      ack = i2c_master_getAck();

      if (ack) {
        os_printf("pcf8574_set_direction(): failed ack after writing addr: 0x%02x \n", addr);
        i2c_master_stop();
        direction = OKARDWARE_I2C_DIRECTION_NONE;
      } else {
        os_printf("pcf8574_set_direction(): selected i2c address: 0x%02x \n", addr);
      }
    }
    pcf8574_current_direction = direction;
  }
  return OKARDWARE_I2C_DIRECTION_NONE != pcf8574_current_direction;
}

bool ICACHE_FLASH_ATTR pcf8574_update()
{
  uint8 ack;

  if (pcf8574_set_direction(OKARDWARE_I2C_DIRECTION_TRANSMITTER)) {
    i2c_master_writeByte(pcf8574_register | pcf8574_read_mask);
    ack = i2c_master_getAck();
    /* os_printf("wrote to pcf: %d [ack=%d]\n", pcf8574_register | pcf8574_read_mask, ack); */

    if (!ack) {
      return true;
    }
    os_printf("pcf8574_update(): failed ack after writing byte: %d \n",
              pcf8574_register | pcf8574_read_mask);

    /* pcf8574_set_direction(OKARDWARE_I2C_DIRECTION_NONE); */
  }
  return false;
}

uint8 pcf8574_register = 0;

/* os_printf("\nREGISTER BIT (%d, %d) before = %d [%8B]", bit, value, (uint32)pcf8574_register, (uint32)pcf8574_register); \ */
/* os_printf(", after = %d [%8B]\n", (uint32)pcf8574_register, (uint32)pcf8574_register, port); */

#define PCF8574_SET_REGISTER_BIT(bit, value)    \
  if (0 == (value)) {                           \
    pcf8574_register &= ~(uint8)(1 << (bit));   \
  } else {                                      \
    pcf8574_register |= 1 << (bit);             \
  }

#define PCF8574_SET_READ_BIT(bit, value)        \
  if (0 == value) {                             \
    pcf8574_read_mask &= ~(uint8)(1 << bit);    \
  } else {                                      \
    pcf8574_read_mask |= 1 << bit;              \
  }


void ICACHE_FLASH_ATTR pcf8574_set(uint8 value)
{
  pcf8574_register = value;
  pcf8574_update();
}

// port is between 0 and 7
void ICACHE_FLASH_ATTR pcf8574_bit(uint8 bit, bool value)
{
  PCF8574_SET_REGISTER_BIT(bit, value);
  pcf8574_update();
}

uint8 ICACHE_FLASH_ATTR pcf8574_read()
{
  pcf8574_set_direction(OKARDWARE_I2C_DIRECTION_RECEIVER);
  uint8 byte = i2c_master_readByte();
  /* pcf8574_register = byte; */
  i2c_master_send_ack();

  pcf8574_set_direction(OKARDWARE_I2C_DIRECTION_NONE);

  return byte;
}

static uint8 _previous_analog_port = -1;

void ICACHE_FLASH_ATTR okardware_select_adc(uint8 port)
{
  if (7 < port) {
    os_printf("invalid ADC port: %d\n", port);
  } else {
    /* if (_previous_analog_port != port) { */
    /* os_printf("\nport[%d], register before = %d [%8B]", port, (uint32)pcf8574_register, (uint32)pcf8574_register); */
    PCF8574_SET_REGISTER_BIT(PCF8574_PORT_CD4051_A, port & 1);
    PCF8574_SET_REGISTER_BIT(PCF8574_PORT_CD4051_B, (port >> 1) & 1);
    PCF8574_SET_REGISTER_BIT(PCF8574_PORT_CD4051_C, (port >> 2) & 1);
    pcf8574_update();
    /* os_printf(", after = %d [%8B]\n", (uint32)pcf8574_register, (uint32)pcf8574_register, port); */
    /*   _previous_analog_port = port; */
    /* } */
  }
}

uint16 ICACHE_FLASH_ATTR okardware_adc_read(uint8 port)
{
  okardware_select_adc(port);

  return system_adc_read();
}

void ICACHE_FLASH_ATTR okardware_adc_print_all()
{
  int i;

  os_printf("ADC values [-1]= %d", system_adc_read());
  for (i = 0; i < 8; i++) {
    os_printf("  [%d]= %d", i, okardware_adc_read(i));
  }
  os_printf("\n");
}

void ICACHE_FLASH_ATTR okardware_dht_on_update(struct sensor_reading *reading)
{
  if (reading->success) {
    // DEBUG
    /* os_printf("DHT ON UPDATE: success=%d,  temp = %d, humid = %d\n", */
    /*           reading->success, reading->temperature, reading->humidity); */

    // Make a CollectD packet for temperature and humidity using UDP binary protocol:
    // https://collectd.org/wiki/index.php/Binary_protocol

    /* unsigned char frame[] = */
    /*   "\x00\x00\x00\x0b" "okabox\0"                // part: HOST (0x0000) */
    /*   "\x00\x08\x00\x0c" "\0\0\0\0\0\0\0\0"        // part: TIME_HR (0x0008) */
    /*   "\x00\x09\x00\x0c" "\x00\x00\x00\x02\x80\x00\x00\x00" // part: INTERVAL_HR (0x0009) */
    /*   "\x00\x02\x00\x08" "dht\0"                   // part: PLUGIN (0x0002) */
    /*   "\x00\x03\x00\x06" "1\0"                     // part: PLUGIN_INSTANCE (0x0003) */
    /*   "\x00\x04\x00\x10" "temperature\0"           // part: TYPE (0x0004) */
    /*   "\x00\x05\x00\x0d" "deci_deg\0"                // part: TYPE_INSTANCE (0x0005) */
    /*   "\x00\x06\x00\x0f" "\x00\x01\x01" "\0\0\0\0\0\0\0\0" // part: VALUES (0x0006) */
    /*   ; */

    /* *(uint32_t *)(frame + 0x0B + 4) = oka_time_get(); */
    /* *(double *)(frame + 0x0B + 0x0C + 0x0C + 0x08 + 0x06 + 0x10 + 0x00 + 0x0F + 7) = */
    /*   (double)reading->temperature / 10; */
    /* /\* int8 frame[] = { 0, 0, 0, 7, 'o', 'k', 'a', 'b', 'o', 'x'} *\/ */
    /* os_printf("sending to UDP port %d a frame with %d bytes\n", oka_udp_collectd->proto.udp->local_port, sizeof(frame)); */
    /* espconn_sendto(oka_udp_collectd, (int8*)&frame, sizeof(frame)); */

    const char buf[64];
    os_sprintf(buf, ":state:temp:%d:%d\n", reading->temperature, reading->humidity);
    oka_udp_send_str(buf);
    okanim_update_temp(reading->temperature);
    okanim_update_humidity(reading->humidity);
  } else {
    okanim_update_temp(-1000);
    okanim_update_humidity(-1);
  }
}

// Rotary from: https://github.com/anupak/esp8266-samplecode/tree/master/i2c_rotary_encoder
/* #define ROTARY_ENCODER_MASK         0b00000111 */

#ifdef ENABLE_ROTARY

typedef enum {NO_CHANGE=0, ENCODER_CW, ENCODER_CCW} encoder_direction_t;

static uint8_t  rotary_input_data[4];
static uint8_t  rotary_counter = 0;

uint8_t decode_rotary_encoder(uint8_t byte, encoder_direction_t *direction, uint8_t *button)
{
  uint8_t ret = 0, temp;
  /* int input = ROTARY_ENCODER_MASK & byte; */
  uint8_t bit1, bit2;

  /* Check if need to process the data in first place */
  *button = 0;
  *direction = NO_CHANGE;

  *button = (byte & (1<< PCF8574_PORT_ROTARY_BUTTON)) >> PCF8574_PORT_ROTARY_BUTTON;
  if (*button) ret += 1;

  /* Sequence CW: 1320, CCW:2310 */
  bit1 = (byte & (1<< PCF8574_PORT_ROTARY_PIN1)) >> PCF8574_PORT_ROTARY_PIN1;
  bit2 = (byte & (1<< PCF8574_PORT_ROTARY_PIN2)) >> PCF8574_PORT_ROTARY_PIN2;

  temp = (bit2)<<1 | bit1;
  if ((rotary_input_data[rotary_counter] != temp))
    {
      if (temp == 0)
        rotary_counter = 0;
      else
        rotary_counter ++;

      rotary_input_data[rotary_counter] = temp;
    }

  if (rotary_counter == 3)
    {
      //os_printf("Rotary Encode: %x %x %x %x ", rotary_input_data[0] , rotary_input_data[1] , rotary_input_data[2] , rotary_input_data[3] );

      if ((rotary_input_data[1] == 1) && (rotary_input_data[3] == 2))
        *direction = ENCODER_CW;

      if ((rotary_input_data[1] == 2) && (rotary_input_data[3] == 1))
        *direction = ENCODER_CCW;

      ret += 1;
      //rotary_counter = 0;
    }



  return (ret);
}

#endif // ENABLE_ROTARY

////////////////////////////////////////////////////////////////////////////////

#ifdef ENABLE_POWER_EMERGENCY

#define EMERGENCY_EXTRA_TIME 3

static volatile os_timer_t watch_timer;

/**
 * This timer is responsible for watching over ampere consumption and
 * create an emergency alert whenever a threshold is exceeded.
 */
void watch_timerfunc(void *arg)
{
  static uint32 _low_power_time = 0;
  static bool   _end_noticed;

  uint32 time = system_get_time();
  uint16 amperes = okardware_adc_read(ADC_AMPERE_USB);

  if (amperes >= LIMIT_AMPERE_USB) {
    if (!_low_power_time) {
      _low_power_time = time;
      _end_noticed = false;
      okadraw_set_low_power(true);
      os_printf("CAREFUL! Too much power drained from USB [%d]\n", amperes);
    }
  } else {
    if (_low_power_time > 0) {
      if (time > _low_power_time + EMERGENCY_EXTRA_TIME * 1000000) {
        _low_power_time = 0;
        okadraw_set_low_power(false);
        os_printf("END OF POWER EMERGENCY [%d]\n", amperes);
      } else if (!_end_noticed) {
        os_printf("POWER BACK :)  (waiting for %d seconds to end emergency) [%d]\n",
                  EMERGENCY_EXTRA_TIME, amperes);
        _end_noticed = true;
      }
    }
  }
}

#endif // ENABLE_POWER_EMERGENCY

static void _trigger_but1()
{
  os_printf("Button press!!  %d\n", GPIO_INPUT_GET(GPIO_ID_PIN(OKA_BUT1_IO_NUM)));

  gpio_pin_intr_state_set(GPIO_ID_PIN(OKA_BUT1_IO_NUM), GPIO_PIN_INTR_ANYEDGE);
}

static void _trigger_pcf()
{
  uint8 val = 0;//pcf8574_read();

  os_printf("PCF interrupt! val = %d -- former = %d\n",
            (uint32)val, (uint32)pcf8574_register);

  gpio_pin_intr_state_set(GPIO_ID_PIN(OKA_PCF8574_INT_IO_NUM), OKA_PCF8574_INT_EDGE);
}

static void _trigger_motion()
{
  os_printf("*** trigger motion!!  %d\n", GPIO_INPUT_GET(GPIO_ID_PIN(OKA_SR501_IO_NUM)));
  gpio_pin_intr_state_set(GPIO_ID_PIN(OKA_SR501_IO_NUM), GPIO_PIN_INTR_ANYEDGE);
}

void    _dispatch_interrupt()
{
  uint32_t gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
  int i;
  int bit;
  for (i=0; i < 15; i++) {
    bit = 1<<i;
    if (gpio_status & bit) {

      // disable interrupt for GPIO
      gpio_pin_intr_state_set(GPIO_ID_PIN(i), GPIO_PIN_INTR_DISABLE);

      /* // ack the intr */
      GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status & bit);

      switch (i) {
      case OKA_PCF8574_INT_IO_NUM: _trigger_pcf(); break;
      case OKA_BUT1_IO_NUM: _trigger_but1(); break;
      case OKA_SR501_IO_NUM: _trigger_motion(); break;
      default:
        os_printf("unknown interrupt on GPIO %d\n", i);
        /* gpio_pin_intr_state_set(GPIO_ID_PIN(i), GPIO_PIN_INTR_DISABLE); */
        /* GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status & bit); */
        gpio_pin_intr_state_set(GPIO_ID_PIN(i), GPIO_PIN_INTR_ANYEDGE);
      }

      /* //clear interrupt status for GPIO0 */
      /* GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status & bit); */

      /* // Reactivate interrupts for GPIO0 */
      /* gpio_pin_intr_state_set(GPIO_ID_PIN(i), GPIO_PIN_INTR_ANYEDGE); */
    }
  }
}

/* typedef struct { */
/* } oka_gpio_ */

char *rst_reasons[] = {
  "poweron", "hard_wdt", "exception", "soft_wdt", "soft_restart", "deeop_sleep_awake", "ext_sys"
};

#define MIN(v1, v2)   ((v1) < (v2) ? (v1) : (v2))

/* gpio_register_set(GPIO_PIN_ADDR(pin_num), GPIO_PIN_INT_TYPE_SET(GPIO_PIN_INTR_DISABLE) \ */
/*                   | GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_DISABLE)  \ */
/*                   | GPIO_PIN_SOURCE_SET(GPIO_AS_PIN_SOURCE));         \ */
#define _INIT_GPIO_INT(pin_num, pin_mux, pin_func, int_type)            \
  PIN_FUNC_SELECT(pin_mux, pin_func);                                   \
  gpio_output_set(0, 0, 0, GPIO_ID_PIN(pin_num));                       \
  GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(pin_num));               \
  gpio_pin_intr_state_set(GPIO_ID_PIN(pin_num), (GPIO_INT_TYPE) int_type);
/* gpio_pin_intr_state_set(GPIO_ID_PIN(pin_num), 3/\*GPIO_PIN_INTR_ANYEGDE*\/); */


void ICACHE_FLASH_ATTR okardware_init()
{
  struct rst_info *rst_info = system_get_rst_info();
  os_printf("BOOTING Okardware [reason=%d,\"%s\" exccause=%u epc1=%u epc2=%u epc3=%u excvaddr=%u depc=%u]\n",
            rst_info->reason, rst_reasons[MIN(sizeof(rst_reasons) / sizeof(*rst_reasons), rst_info->reason)],
            rst_info->exccause, rst_info->epc1, rst_info->epc2, rst_info->epc3,
            rst_info->excvaddr, rst_info->depc);

  gpio_init();

#ifdef ENABLE_OKARDWARE_PCF8574
  /* PCF8574_SET_READ_BIT(0, 1); */
  /* PCF8574_SET_READ_BIT(1, 1); */
  i2c_master_gpio_init();
#endif

#ifdef ENABLE_POWER_EMERGENCY
  // Watch
  os_timer_disarm(&watch_timer);
  os_timer_setfn(&watch_timer, (os_timer_func_t *)watch_timerfunc, NULL);
  /* os_timer_arm(&watch_timer, 500, 1); */
#endif // ENABLE_POWER_EMERGENCY

  // Sensors
  dht_update_cb = &okardware_dht_on_update;
  DHTInit(SENSOR_DHT22, 5000);

  // Interrupts
  ETS_GPIO_INTR_DISABLE();
  ETS_GPIO_INTR_ATTACH(_dispatch_interrupt, NULL);

  _INIT_GPIO_INT(OKA_SR501_IO_NUM, OKA_SR501_IO_MUX, OKA_SR501_IO_FUNC, GPIO_PIN_INTR_ANYEDGE);
  PIN_PULLUP_DIS(OKA_SR501_IO_MUX);

#if 0
  _INIT_GPIO_INT(OKA_BUT1_IO_NUM, OKA_BUT1_IO_MUX, OKA_BUT1_IO_FUNC, GPIO_PIN_INTR_ANYEDGE);
  PIN_PULLUP_EN(OKA_BUT1_IO_MUX);
/* #endif */
/* #ifdef ENABLE_OKARDWARE_PCF8574 */
  _INIT_GPIO_INT(OKA_PCF8574_INT_IO_NUM, OKA_PCF8574_INT_IO_MUX, OKA_PCF8574_INT_IO_FUNC, OKA_PCF8574_INT_EDGE);
  /* PIN_PULLDWN_DIS(OKA_PCF8574_INT_IO_MUX);                             // Disable pulldown */
  PIN_PULLUP_EN(OKA_PCF8574_INT_IO_MUX);                               // Enable pullup
  /* PIN_PULLUP_DIS(OKA_PCF8574_INT_IO_MUX); */
#endif
  ETS_GPIO_INTR_ENABLE();


  //////////////////////////////////////////////////////////////////////

  /* single_key[0] = key_init_single(OKA_BUT1_IO_NUM, OKA_BUT1_IO_MUX, OKA_BUT2_IO_FUNC, */
  /*                                 _long_press, _short_press); */

  /* keys.key_num = KEY_NUM; */
  /* keys.single_key = single_key; */
  /* key_init(&keys); */
}

/* void ICACHE_FLASH_ATTR okardware_scan() */
/* { */
/*   uint8 addr; */

/*   for (addr = 0; addr <= 255; addr++) { */

/*     uint8 ack; */

/*     os_printf("trying: %d, %x\n", addr, addr); */
/*     i2c_master_start(); */
/*     i2c_master_writeByte(addr); */
/*     ack = i2c_master_getAck(); */
/*     if (ack) { */
/*     } else { */
/*       os_printf("yes: %d, %x !!!!!\n", addr, addr); */
/*     } */
/*     i2c_master_stop(); */
/*     system_soft_wdt_feed(); */
/*     i2c_master_wait(40000); */
/*   } */
/*   os_printf("scan done.\n"); */
/* } */


bool ICACHE_FLASH_ATTR okardware_check(uint8 byte)
{
  bool status;

  os_printf("Checking okardware...\n");

  struct sensor_reading *dht = readDHT(0);
  os_printf("DHT success=%d,  temp = %d, humid = %d\n",
            dht->success, (int)(1000 * dht->temperature), (int)(1000 * dht->humidity));
}

uint32 ICACHE_FLASH_ATTR okardware_read_temp()
{
  struct sensor_reading *dht = readDHT(1);

  os_printf("DHT success=%d,  temp = %d, humid = %d\n",
            dht->success, (int)(1000 * dht->temperature), (int)(1000 * dht->humidity));

  return (int)(1000 * dht->temperature);
}

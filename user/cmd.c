// https://github.com/anupak/esp8266-samplecode/tree/master/uart

#include "c_types.h"
#include "osapi.h"
#include "user_interface.h"
#define UART_BUFF_EN 1
#include "driver/uart.h"
#include "anim.h"
#include "okardware.h"
#include "okatime.h"
#include "cmd.h"

void ICACHE_FLASH_ATTR
oka_cmd_console_init(int8 debug_level)
{
  uart_init(BIT_RATE_115200, BIT_RATE_115200);
  /* uart_div_modify(0, UART_CLK_FREQ / 115200); */

  // ETS_UART_INTR_ATTACH() from ets_sys.h

  if (OKA_LEVEL_NOTICE <= debug_level) {
    os_printf("\n\n*** Starting OKABOX...\n\n");
  }

  if (OKA_LEVEL_INFO <= debug_level) {
    oka_cmd_print_info(debug_level);
  }
}

void ICACHE_FLASH_ATTR oka_cmd_print_info(int8 debug_level)
{
  os_printf("ChipID = %u, flash = %u, SDK = %s, free heap = %u, vdd33 = 0x%4x\n",
            system_get_chip_id(), spi_flash_get_id(), system_get_sdk_version(),
            system_get_free_heap_size(), system_get_vdd33());
  os_printf("Time is: %s ;; system time = %u\n", oka_time_format(), system_get_time());

  if (OKA_LEVEL_DEBUG <= debug_level) {
    system_print_meminfo();
  }
}



static int ICACHE_FLASH_ATTR _cmd_print_commands(size_t argc, char **argv, uint16 _command_idx);
static int ICACHE_FLASH_ATTR _cmd_info(size_t argc, char **argv, uint16 _command_idx);
static int ICACHE_FLASH_ATTR _cmd_restart(size_t argc, char **argv, uint16 _command_idx);
static int ICACHE_FLASH_ATTR _cmd_sync(size_t argc, char **argv, uint16 _command_idx);
static int ICACHE_FLASH_ATTR _cmd_set_time(size_t argc, char **argv, uint16 _command_idx);
static int ICACHE_FLASH_ATTR _cmd_read_temp(size_t argc, char **argv, uint16 _command_idx);
static int ICACHE_FLASH_ATTR _cmd_read_adc(size_t argc, char **argv, uint16 _command_idx);
static int ICACHE_FLASH_ATTR _cmd_bg(size_t argc, char **argv, uint16 _command_idx);
static int ICACHE_FLASH_ATTR _cmd_random(size_t argc, char **argv, uint16 _command_idx);
static int ICACHE_FLASH_ATTR _cmd_test(size_t argc, char **argv, uint16 _command_idx);
static int ICACHE_FLASH_ATTR _cmd_wdt(size_t argc, char **argv, uint16 _command_idx);
static int ICACHE_FLASH_ATTR _cmd_write_pcf(size_t argc, char **argv, uint16 _command_idx);
static int ICACHE_FLASH_ATTR _cmd_read_pcf(size_t argc, char **argv, uint16 _command_idx);
static int ICACHE_FLASH_ATTR _print_commands();
/* static int _cmd_print_commands(size_t argc, char **argv, uint16 _command_idx); */
/* static int _cmd_bg(size_t argc, char **argv, uint16 _command_idx); */
/* static int _print_commands(); */

typedef struct _command  {
  int  (*handler)(size_t argc, char **argv, uint16 _command_idx);
  char  *name;
  char  *usage;
} oka_command_t;

static oka_command_t _commands[] = {
  {
  handler: &_cmd_print_commands,
  name: "help",
  usage: ""
  },
  {
  handler: &_cmd_info,
  name: "info",
  usage: ""
  },
  {
  handler: &_cmd_restart,
  name: "restart",
  usage: ""
  },
  {
  handler: &_cmd_sync,
  name: "sync",
  usage: ""
  },
  {
  handler: &_cmd_set_time,
  name: "set-time",
  usage: ""
  },
  {
  handler: &_cmd_read_temp,
  name: "read-temp",
  usage: ""
  },
  {
  handler: &_cmd_read_adc,
  name: "read-adc",
  usage: ""
  },
  {
  handler: &_cmd_bg,
  name: "bg",
  usage: "{ 0-2 | hsv:<hue>:<saturation>:<value> }"
  },
  {
  handler: &_cmd_random,
  name: "random",
  usage: "{ 0-2 | random:<property>[:<value>] }"
  },
  {
  handler: &_cmd_write_pcf,
  name: "write-pcf",
  usage: ""
  },
  {
  handler: &_cmd_read_pcf,
  name: "read-pcf",
  usage: ""
  },
  {
  handler: &_cmd_test,
  name: "test",
  usage: ""
  },
  {
  handler: &_cmd_test,
  name: "wdt",
  usage: ""
  },
};

static uint16 _nb_commands = sizeof(_commands) / sizeof(*_commands);

void ICACHE_FLASH_ATTR
oka_cmd_shell_init()
{
  /* uart_rx_intr_enable(UART0); */

  /* uart_init(0); */
  /* uart_tx_one_char(UART0, '\n'); */

  uint8 uart_buf[128]={0};
  uint16 len = 0;
  len = rx_buff_deq(uart_buf, 128 );
  /* tx_buff_enq(uart_buf,len); */

  /* uart_buf[len] = '\0'; */
  os_printf("Got: %s\n", uart_buf);

  _print_commands();

}

void ICACHE_FLASH_ATTR oka_cmd_eval(const char *command, size_t len)
{
  char  buf[512];
  char *argv[32] = { buf };
  int   argc = 0;
  /* int   last_idx = 0; */
  int   i;
  int status;

  // INFO
  /* os_printf("OKA eval command: %s\n", command); */
  if (len > 511) {
    len = 511;
  }

  os_strncpy(buf, command, len);
  buf[len] = '\0';
  /* if (0 < len && '\n' == buf[len - 1]) { */
  /*   buf[len - 1] = '\0'; */
  /* } */

  for (i = 0; i < len; i++) {
    if ('\r' == buf[i] || '\n' == buf[i]) {
      buf[i] = '\0';
      break;
    } else if (':' == buf[i]) {
      buf[i] = '\0';
      argc++;
      argv[argc] = buf + i + 1;
      /* os_printf("got arg %d [pos %d]: %s\n", argc, i, buf + i + 1); */
    }
  }

  /* for (i = 0; i <= argc; i++) { */
  /*   os_printf("argv[%d] = %s\n", i, argv[i]); */
  /* } */

  for (i = 0; i < _nb_commands; i++) {
    if (!os_strcmp(_commands[i].name, argv[0])) {
      status = _commands[i].handler(argc, argv, i);
      if (0 != status) {
        os_printf("Command <%s> done with status %d.\n", argv[0], status);
      }
      break;
    }
  }
  if (i == _nb_commands) {
    os_printf("invalid command: %s\n", argv[0]);
  }
}

////////////////////////////////////////////////////////////////////////////////

#define _CMD_ERROR_USAGE(_cmd_idx)                                      \
  os_printf("Bad usage! Usage: %s %s\n",                                \
            _commands[_cmd_idx].name, _commands[_cmd_idx].usage);       \
  return 1


static int ICACHE_FLASH_ATTR _cmd_print_commands(size_t argc, char **argv, uint16 _command_idx)
{
  _print_commands();
  return 0;
}

static int ICACHE_FLASH_ATTR _print_commands()
{
  uint16_t i;
  os_printf("SHELL available commands: ");

  for (i = 0; i < _nb_commands; i++) {
    os_printf("%s%s", _commands[i].name, i == _nb_commands - 1 ? "" : ", ");
  }
  os_printf("\n");
}

static int ICACHE_FLASH_ATTR _cmd_info(size_t argc, char **argv, uint16 _command_idx)
{
  os_printf("-- System information --\n");
  oka_cmd_print_info(OKA_LEVEL_DEBUG);
  return 0;
}

static int ICACHE_FLASH_ATTR _cmd_restart(size_t argc, char **argv, uint16 _command_idx)
{
  os_printf("-- Restarting chip...\n");
  system_restart();
  return 2;
}

static int ICACHE_FLASH_ATTR _cmd_sync(size_t argc, char **argv, uint16 _command_idx)
{
  oka_time_sync();
  return 0;
}

static int ICACHE_FLASH_ATTR _cmd_set_time(size_t argc, char **argv, uint16 _command_idx)
{
  time_t ts;

  if (6 <= argc) {
    /* struct tm tm = { */
    /*   .tm_year = atoi(argv[1]), */
    /*   .tm_mon = atoi(argv[2]), */
    /*   .tm_mday = atoi(argv[3]), */
    /*   .tm_hour = atoi(argv[4]), */
    /*   .tm_min = atoi(argv[5]), */
    /*   .tm_sec = atoi(argv[6]), */
    /* }; */
    /* ts = mktime(&tm); */
    ts = system_mktime(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]),
                       atoi(argv[4]), atoi(argv[5]), atoi(argv[6]));

  } else if (1 <= argc) {
    ts = atoi(argv[1]);
  }
  oka_time_update(&ts);
  /* os_printf("Timestamp updated to: %d\n", ts); */
  return 0;
}

static int ICACHE_FLASH_ATTR _cmd_read_temp(size_t argc, char **argv, uint16 _command_idx)
{
  okardware_read_temp();
  return 0;
}

static int ICACHE_FLASH_ATTR _cmd_read_adc(size_t argc, char **argv, uint16 _command_idx)
{
  int8_t arg = -2;

  if (1 <= argc) {
    arg = atoi(argv[1]);
  }

  if (-1 <= arg) {
    uint16_t value = -1 == arg ? system_adc_read() : okardware_adc_read(arg);
    os_printf("ADC [%d] value = %d\n", arg, value);
  } else {
    okardware_adc_print_all();
  }
  return 0;
}

#include "driver/httpclient.h"

/* static char headers[1000] = { 0 }; */

static void _http_callback_example(char * response, int http_status, char * full_response)
{
  os_printf("*** HTTP response! status = %d\n%s\n", http_status, full_response);
}

static int ICACHE_FLASH_ATTR _cmd_test(size_t argc, char **argv, uint16 _command_idx)
{
  /* uint8 arg = 0; */

  /* if (2 <= argc) { */
  /*   okanim_update_temp(atoi(argv[1])); */
  /*   okanim_update_humidity(atoi(argv[2])); */
  /* } */
  /* oka_udp_send_str("yoooooo"); */
  /* return okardware_check(arg) ? 0 : 2; */
  if (argc >= 1) {
    /* http_get(argv[1], "", &_http_callback_example); */
    return 0;
  }
  _CMD_ERROR_USAGE(_command_idx);
}

static int ICACHE_FLASH_ATTR _cmd_wdt(size_t argc, char **argv, uint16 _command_idx)
{
  os_printf("Starting infinite loop...\n");
  while (42) {}
  return 2;
}

static int ICACHE_FLASH_ATTR _cmd_write_pcf(size_t argc, char **argv, uint16 _command_idx)
{
  uint8 arg = 0;

  if (1 <= argc) {
    arg = atoi(argv[1]);
  }
  if (2 <= argc) {
    pcf8574_bit(arg, atoi(argv[2]));
  } else {
    pcf8574_set(arg);
  }
  os_printf("new pcf854_register = %d [%08B]\n", (uint32)pcf8574_register);
  return 0;
}

static int ICACHE_FLASH_ATTR _cmd_read_pcf(size_t argc, char **argv, uint16 _command_idx)
{
  uint32 val = pcf8574_read();

  os_printf("Read PCF8574 value = %d [0x%02x, 0B%08B]\n", val, val, val);

  return 0;
}

static int ICACHE_FLASH_ATTR _cmd_bg(size_t argc, char **argv, uint16 _command_idx)
{

  if (1 <= argc) {
    if ('0' <= argv[1][0] && '9' >= argv[1][0]) {
      uint8 bg = atoi(argv[1]);
      os_printf("Setting BG: %d\n", bg);
      oka_anim_set_bg(bg);
      return 0;

    } else if (!os_strcmp("hsv", argv[1])) {
      if (4 <= argc) {
        uint8 hsv[0];
        uint8 i;
        for (i = 0; i < 3; i++) {
          hsv[i] = atoi(argv[2 + i]);
        }
        os_printf("Setting BG: hsv(%u, %u, %u)\n", hsv[0], hsv[1], hsv[2]);
        oka_anim_set_bg_hsv(hsv[0], hsv[1], hsv[2]);
        return 0;
      }
    }
  }
  _CMD_ERROR_USAGE(_command_idx);
}

static int ICACHE_FLASH_ATTR _cmd_random(size_t argc, char **argv, uint16 _command_idx)
{
  if (2 <= argc) {
    uint32 val = atoi(argv[2]);

    if (!strcmp("h1", argv[1])) {
      okanim_fx_random.color1.h = val;
    } else if (!strcmp("h2", argv[1])) {
      okanim_fx_random.color2.h = val;
    } else if (!strcmp("s1", argv[1])) {
      okanim_fx_random.color1.s = val;
    } else if (!strcmp("s2", argv[1])) {
      okanim_fx_random.color2.s = val;
    } else if (!strcmp("v1", argv[1])) {
      okanim_fx_random.color1.v = val;
    } else if (!strcmp("v2", argv[1])) {
      okanim_fx_random.color2.v = val;
    } else if (!strcmp("start_idx", argv[1])) {
      okanim_fx_random.start_idx = val;
    } else if (!strcmp("end_idx", argv[1])) {
      okanim_fx_random.end_idx = val;
    } else {
      os_printf("invalid_property: %s\n", argv[1]);
      _CMD_ERROR_USAGE(_command_idx);
    }
    return 0;
  }
  _CMD_ERROR_USAGE(_command_idx);
}

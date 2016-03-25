// From: https://github.com/mathew-hall/esp8266-dht/blob/master/user/dht.c
/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you
 * retain
 * this notice you can do whatever you want with this stuff. If we meet some
 * day,
 * and you think this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#include "ets_sys.h"
#include "osapi.h"
/* #include "espmissingincludes.h" */
#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "gpio.h"
#include "dht.h"
#include "user_config.h"

#define MAXTIMINGS 10000
#define DHT_MAXCOUNT 32000
#define BREAKTIME 32

/* #define DHT_PIN 12 */

enum sensor_type SENSOR;

/* static inline float scale_humidity(int *data) { */
/*   if (SENSOR == SENSOR_DHT11) { */
/*     return data[0]; */
/*   } else { */
/*     float humidity = data[0] * 256 + data[1]; */
/*     return humidity /= 10; */
/*   } */
/* } */

/* static inline float scale_temperature(int *data) { */
/*   if (SENSOR == SENSOR_DHT11) { */
/*     return data[2]; */
/*   } else { */
/*     float temperature = data[2] & 0x7f; */
/*     temperature *= 256; */
/*     temperature += data[3]; */
/*     /\* temperature /= 10; *\/ */
/*     if (data[2] & 0x80) */
/*       temperature *= -1; */
/*     return temperature; */
/*   } */
/* } */

static inline void delay_ms(int sleep) {
  os_delay_us(1000 * sleep);
}

static struct sensor_reading reading = {
  .source = "DHT11", .success = 0
};

void     (*dht_update_cb)(struct sensor_reading *reading) = NULL;



/**
    Originally from: http://harizanov.com/2014/11/esp8266-powered-web-server-led-control-dht22-temperaturehumidity-sensor-reading/
    Adapted from: https://github.com/adafruit/Adafruit_Python_DHT/blob/master/source/Raspberry_Pi/pi_dht_read.c
    LICENSE:
    // Copyright (c) 2014 Adafruit Industries
    // Author: Tony DiCola

    // Permission is hereby granted, free of charge, to any person obtaining a copy
    // of this software and associated documentation files (the "Software"), to deal
    // in the Software without restriction, including without limitation the rights
    // to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    // copies of the Software, and to permit persons to whom the Software is
    // furnished to do so, subject to the following conditions:

    // The above copyright notice and this permission notice shall be included in all
    // copies or substantial portions of the Software.

    // THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    // IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    // FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    // AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    // LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    // OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    // SOFTWARE.
    */
static  void ICACHE_FLASH_ATTR pollDHTCb(void * arg){
  int counter = 0;
  int laststate = 1;
  int i = 0;
  int bits_in = 0;
  // int bitidx = 0;
  // int bits[250];

  int data[100];

  data[0] = data[1] = data[2] = data[3] = data[4] = 0;

  //disable interrupts, start of critical section
  ets_intr_lock();
  system_soft_wdt_feed();
  /* wdt_feed(); */

  // Wake up device, 250ms of high
  GPIO_OUTPUT_SET(OKA_DHT_IO_NUM, 1);
  delay_ms(20);

  // Hold low for 20ms
  GPIO_OUTPUT_SET(OKA_DHT_IO_NUM, 0);
  delay_ms(20);

  // High for 40ms
  // GPIO_OUTPUT_SET(2, 1);

  GPIO_DIS_OUTPUT(OKA_DHT_IO_NUM);
  os_delay_us(40);

  // Set pin to input with pullup
  // PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO2_U);

  // os_printf("Waiting for GPIO to drop \n");

  // wait for pin to drop?
  while (GPIO_INPUT_GET(OKA_DHT_IO_NUM) == 1 && i < DHT_MAXCOUNT) {
    if (i >= DHT_MAXCOUNT) {
      goto fail;
    }
    i++;
  }

  //  os_printf("Reading DHT\n");

  // read data!
  for (i = 0; i < MAXTIMINGS; i++) {
    // Count high time (in approx us)
    counter = 0;
    while (GPIO_INPUT_GET(OKA_DHT_IO_NUM) == laststate) {
      counter++;
      os_delay_us(1);
      if (counter == 1000)
        break;
    }
    laststate = GPIO_INPUT_GET(OKA_DHT_IO_NUM);

    if (counter == 1000)
      break;

    // store data after 3 reads
    if ((i > 3) && (i % 2 == 0)) {
      // shove each bit into the storage bytes
      data[bits_in / 8] <<= 1;
      if (counter > BREAKTIME) {
        //os_printf("1");
        data[bits_in / 8] |= 1;
      } else {
        //os_printf("0");
      }
      bits_in++;
    }
  }

  //Re-enable interrupts, end of critical section
  ets_intr_unlock();

  if (bits_in < 40) {
    // INFO
    os_printf("DHT sensor: Got too few bits: %d should be at least 40\n", bits_in);
    goto fail;
  }


  int checksum = (data[0] + data[1] + data[2] + data[3]) & 0xFF;

  // DEBUG
  /* os_printf("DHT: %02x %02x %02x %02x [%02x] CS: %02x\n", data[0], data[1],data[2],data[3],data[4],checksum); */

  if (data[4] != checksum) {
    os_printf("DHT sensor: Checksum was incorrect after %d bits. Expected %d but got %d",
              bits_in, data[4], checksum);
    goto fail;
  }

  reading.humidity = data[0] * 256 + data[1];
  reading.temperature = (data[2] & 0x7f) * 256 + data[3];
  if (data[2] & 0x80) {
    reading.temperature *= -1;
  }

  /* reading.humidity = (data[0] << 8) | data[1]; */
  /* reading.temperature = (data[2] << 7) | data[3]; */

  /* reading.humidity = ((int16_t *)data)[0]; */
  /* reading.temperature = ((int16_t *)data)[1]; */
  /* reading.temperature = scale_temperature(data); */
  /* reading.humidity = scale_humidity(data); */
  // DEBUG
  /* os_printf("Temp =  %d *C, Hum = %d %% -- 0x%04x\n", reading.temperature, reading.humidity, *(int32_t*)data); */

  /* os_printf("Temp =  %d *C, Hum = %d %%\n", (int)(reading.temperature * 100), */
  /*           (int)(reading.humidity * 100)); */

  reading.success = 1;
  if (dht_update_cb) {
    dht_update_cb(&reading);
  }
  return;
 fail:
  reading.success = 0;
  if (dht_update_cb) {
    dht_update_cb(&reading);
  }
  /* os_printf("DHT sensor: Failed to get reading, dying\n"); */
}

struct sensor_reading *ICACHE_FLASH_ATTR readDHT(int force) {
  if(force){
    pollDHTCb(NULL);
  }
  return &reading;
}

void DHTInit(enum sensor_type sensor_type, uint32_t polltime) {
  SENSOR = sensor_type;
  // Set OKA_DHT_IO_NUM to output mode for DHT22
  PIN_FUNC_SELECT(OKA_DHT_IO_MUX, OKA_DHT_IO_FUNC);
  PIN_PULLUP_EN(OKA_DHT_IO_MUX);
  /* PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0); */

  os_printf("DHT sensor: setup for type %d, poll interval of %d\n", sensor_type, (int)polltime);

  static ETSTimer dhtTimer;
  os_timer_setfn(&dhtTimer, pollDHTCb, NULL);
  os_timer_arm(&dhtTimer, polltime, 1);
}

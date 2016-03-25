#include "osapi.h"
#include "os_type.h"
#include "ws2812.h"
#include "okatime.h"

#include "anim.h"

static uint8_t _it = 0;

////////////////////////////////////////////////////////////////////////////////
// Permanent FX

okadraw_hsv_gradient_t okanim_fx_random = {
  /* static okadraw_hsv_gradient_t _static_random = { */
  .color1 = { h: 120, s: 255, v: 150 },
  .color2 = { h: 0, s: 255, v: 150 },
  .start_idx = 0,
  .end_idx = 5,
  .mirror = true
};

static okadraw_time_fx_t _time_fx_random = {
  .start_time = 0,
  .loops = 0,
  .duration = 2000,
  .easing = &okadraw_easing_linear_back,
  /* .start_value = 0, */
  /* .end_value = 10, */
  /* .ptr = &okanim_fx_random.end_idx, */
  /* .ptr_type = OKADRAW_PROP_INT16 */
  .start_value = 0,
  .end_value = 80,
  .ptr = &okanim_fx_random.color2.h,
  .ptr_type = OKADRAW_PROP_INT8
};

static okadraw_hsv_gradient_t _clock_gradient = {
  .color1 = { h: 0, s: 255, v: 255 },
  .color2 = { h: 0, s: 255, v: 255 },
  .start_idx = 50,
  .end_idx = 59,
  .mirror = true
};
static okadraw_hsv_gradient_t _clock_sec_gradient = {
  .color1 = { h: 0, s: 255, v: 255 },
  .color2 = { h: 0, s: 255, v: 50 },
  .start_idx = 49,
  .end_idx = 49,
  .mirror = true
};

static okadraw_time_fx_t _clock_time_fx = {
  .start_time = 0,
  .loops = 0,
  .duration = 2000,
  .easing = &okadraw_easing_linear_back,
  .start_value = 1,
  .end_value = 255,
  .ptr = &_clock_sec_gradient.color1.v,
  .ptr_type = OKADRAW_PROP_INT8
};
static okadraw_time_fx_t _clock_time_fx2 = {
  .start_time = 0,
  .loops = 0,
  .duration = 6000,
  .easing = &okadraw_easing_linear,
  .start_value = 0,
  .end_value = 255,
  .ptr = &_clock_sec_gradient.color1.h,
  .ptr_type = OKADRAW_PROP_INT8
};

static okadraw_hsv_gradient_t _temp_gradient = {
  .color1 = { h: 0, s: 255, v: 0 },
  /* .color2 = { h: 0, s: 255, v: 255 }, */
  .start_idx = 0, // 17 is 0 degrees
  .end_idx = 0,
  .mirror = false
};
static okadraw_hsv_gradient_t _humid_gradient = {
  .color1 = { h: 0, s: 255, v: 0 },
  /* .color2 = { h: 0, s: 255, v: 255 }, */
  .start_idx = 0,
  .end_idx = 0,
  .mirror = false
};

////////////////////////////////////////////////////////////////////////////////

static volatile os_timer_t clock_timer;

#define _CLOCK_CYCLE_HOURS 12

void ICACHE_FLASH_ATTR clock_timer_trigger()
{
  struct tm *time = oka_time_get_tm();
  _clock_gradient.color2.h = OKA_LINEAR_PROP
    (20, 160,
     time->tm_sec + time->tm_min * 60 + (time->tm_hour % _CLOCK_CYCLE_HOURS) * 3600,
     0, 3600 * _CLOCK_CYCLE_HOURS - 1);
  /* os_printf("linear(0, 255, %d, %d, %d) = %d\n", */
  /*           time->tm_sec + time->tm_min * 60 + (time->tm_hour % _CLOCK_CYCLE_HOURS) * 3600, */
  /*           0, 3600 * _CLOCK_CYCLE_HOURS - 1, _clock_gradient.color2.h); */

  _clock_gradient.color1.h = (_clock_gradient.color2.h + 10) % 256;
}

void ICACHE_FLASH_ATTR oka_anim_start()
{
  okadraw_add((okadraw_func_draw_t)&okadraw_draw_time_fx, &_time_fx_random);
  okadraw_add((okadraw_func_draw_t)&okadraw_draw_hsv_gradient, &okanim_fx_random);

  okadraw_add((okadraw_func_draw_t)&okadraw_draw_time_fx, &_clock_time_fx);
  okadraw_add((okadraw_func_draw_t)&okadraw_draw_time_fx, &_clock_time_fx2);
  okadraw_add((okadraw_func_draw_t)&okadraw_draw_hsv_gradient, &_clock_gradient);
  okadraw_add((okadraw_func_draw_t)&okadraw_draw_hsv_gradient, &_clock_sec_gradient);

  /* okadraw_add((okadraw_func_draw_t)&okadraw_draw_time_fx, &_temp_time_fx); */
  okadraw_add((okadraw_func_draw_t)&okadraw_draw_hsv_gradient, &_temp_gradient);
  okadraw_add((okadraw_func_draw_t)&okadraw_draw_hsv_gradient, &_humid_gradient);

  os_timer_disarm(&clock_timer);
  os_timer_setfn(&clock_timer, (os_timer_func_t *)clock_timer_trigger, NULL);
  os_timer_arm(&clock_timer, 300, 1);
}



#define MAX_IDX   10

void ICACHE_FLASH_ATTR
oka_anim_samsung_sensors(double *vals, size_t nb_vals)
{
  // 0 = time ?
  // 2, 3, 4
  // 6, 7, 8
  // 10, 11, 12

  /* os_printf("sign: %d -- %d\n", vals[2] > 0 ? 1 : -1, (int)((vals[2] / 12 + 2) * 127)); */
  WS2812SetHSV(80, (vals[2] / 12 + 2) * 127, 255, 255);
  WS2812SetHSV(81, (vals[3] / 12 + 2) * 127, 255, 255);
  WS2812SetHSV(82, (vals[4] / 12 + 2) * 127, 255, 255);

  WS2812SetHSV(90, (vals[6] / 12 + 2) * 127, 255, 255);
  WS2812SetHSV(91, (vals[7] / 12 + 2) * 127, 255, 255);
  WS2812SetHSV(92, (vals[8] / 12 + 2) * 127, 255, 255);

  WS2812SetHSV(110, (vals[10] / 12 + 2) * 127, 255, 255);
  WS2812SetHSV(111, (vals[11] / 12 + 2) * 127, 255, 255);
  WS2812SetHSV(112, (vals[12] / 12 + 2) * 127, 255, 255);

}


////////////////////////////////////////////////////////////////////////////////

void ICACHE_FLASH_ATTR oka_anim_set_bg(uint8 bg)
{
  HsvColor color;
  switch (bg) {
  case 0: color = HSV_COLOR(0, 0, 0); break;
  case 1: color = HSV_COLOR(0, 0, 50); break;
  case 2: color = HSV_COLOR(120, 255, 50); break;
  }

  okadraw_bg.color = color;
}

void ICACHE_FLASH_ATTR oka_anim_set_bg_hsv(uint8 h, uint8 s, uint8 v)
{
  okadraw_bg.color = HSV_COLOR(h, s, v);
}

void ICACHE_FLASH_ATTR okanim_update_temp(int16_t temperature)
{
  if (temperature > -1000) {
    _temp_gradient.color1.v = 255;
    _temp_gradient.color1.h = //_temp_gradient.color2.h =
      OKA_LINEAR_PROP(160, 0, temperature, 100, 300);

    _temp_gradient.start_idx = _temp_gradient.end_idx = 17 + temperature * 29 / 1000;
    /* (17 + temperature * 29 / 1000) % STRIP_LENGTH; */

    /* os_printf("update_temp: h = %d, idx = %d -- temp = %d, %d -- humid = %d, idx = %d\n", */
    /*           (int)_temp_gradient.color1.h, (int)_temp_gradient.start_idx, */
    /*           temperature, 17 + temperature * 29 / 1000); */
  } else {
    _temp_gradient.color1.v = 0;
  }
}

void ICACHE_FLASH_ATTR okanim_update_humidity(int16_t humidity)
{
  if (humidity >= 0) {
    _humid_gradient.color1.v = 255;

    _humid_gradient.color1.h = //_humid_gradient.color2.h =
      OKA_LINEAR_PROP(160, 0, humidity, 100, 300);

    _humid_gradient.start_idx = _humid_gradient.end_idx =
      STRIP_LENGTH - 1 - (17 + humidity * 29 / 1000);

    /* os_printf("update_temp: humid = %d, idx = %d\n", humidity, */
    /*           STRIP_LENGTH - 1 - (17 + humidity * 29 / 1000)); */
  } else {
    _humid_gradient.color1.v = 0;
  }
}

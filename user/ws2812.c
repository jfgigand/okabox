#include "ets_sys.h"
#include "osapi.h"
#include "ws2812.h"

char    ws2812_buffer[WS2812_BUFFER_SIZE];


/**
 * Comment from JFG <jf@geonef.fr>
 *
 * This was imported from NodeMCU: nodemcu-firmware/app/modules/ws2812.c in 2015-2016.
 * Thanks guys :)
 */

// eagle_soc.h
#define PERIPHS_GPIO_BASEADDR               0x60000300
#define ETS_UNCACHED_ADDR(addr) (addr)
#define WRITE_PERI_REG(addr, val) (*((volatile uint32_t *)ETS_UNCACHED_ADDR(addr))) = (uint32_t)(val)
#define GPIO_REG_WRITE(reg, val)                 WRITE_PERI_REG(PERIPHS_GPIO_BASEADDR + reg, val)
#define GPIO_OUT_W1TS_ADDRESS             0x04
#define GPIO_OUT_W1TC_ADDRESS             0x08

/**
 * All this code is mostly from http://www.esp8266.com/viewtopic.php?f=21&t=1143&sid=a620a377672cfe9f666d672398415fcb
 * from user Markus Gritsch.
 * I just put this code into its own module and pushed into a forked repo,
 * to easily create a pull request. Thanks to Markus Gritsch for the code.
 */

// ----------------------------------------------------------------------------
// -- This WS2812 code must be compiled with -O2 to get the timing right.  Read this:
// -- http://wp.josh.com/2014/05/13/ws2812-neopixels-are-not-so-finicky-once-you-get-to-know-them/
// -- The ICACHE_FLASH_ATTR is there to trick the compiler and get the very first pulse width correct.
static void ICACHE_FLASH_ATTR __attribute__((optimize("O2"))) send_ws_0(uint8_t gpio){
  uint8_t i;
  i = 4; while (i--) GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, 1 << gpio);
  i = 9; while (i--) GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, 1 << gpio);
}

static void ICACHE_FLASH_ATTR __attribute__((optimize("O2"))) send_ws_1(uint8_t gpio){
  uint8_t i;
  i = 8; while (i--) GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, 1 << gpio);
  i = 6; while (i--) GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, 1 << gpio);
}

void ICACHE_FLASH_ATTR WS2812OutBuffer(const char *buffer, size_t length)
{
  send_ws_0(WSGPIO);
  os_delay_us(10);

  ets_intr_lock();
  const char * const end = buffer + length;
  while (buffer != end) {
    uint8_t mask = 0x80;
    while (mask) {
      (*buffer & mask) ? send_ws_1(WSGPIO) : send_ws_0(WSGPIO);
      mask >>= 1;
    }
    ++buffer;
  }
  ets_intr_unlock();
}

////////////////////////////////////////////////////////////////////////////////

#define OKADRAW_MAX_FUNCS       64

static okadraw_func_t   okadraw_funcs[OKADRAW_MAX_FUNCS];
static size_t           okadraw_funcs_count = 0;

void okadraw_add(okadraw_func_draw_t func, void *arg)
{
  okadraw_funcs[okadraw_funcs_count].draw = func;
  okadraw_funcs[okadraw_funcs_count].arg = arg;
  okadraw_funcs[okadraw_funcs_count].active = 1;

  okadraw_funcs_count++;
}

void okadraw_remove(okadraw_func_draw_t func, void *arg)
{
  int idx = okadraw_find(func, arg);
  if (idx == okadraw_funcs_count - 1) {
    okadraw_funcs_count--;
  } else {
    okadraw_funcs[idx].active = 0;
  }
}

int okadraw_find(okadraw_func_draw_t func, void *arg)
{
  int i;
  for (i = 0; i < okadraw_funcs_count; i++) {
    if (okadraw_funcs[i].draw == func && okadraw_funcs[i].arg == arg) {
      return i;
    }
  }
  return -1;
}

static bool okadraw_low_power_enabled = false;

int okadraw_redraw()
{
  uint32 mtime = system_get_time() / 1000;
  int i;
  /* for (i = okadraw_funcs_count - 1; i >= 0; i--) { */
  for (i = 0; i < okadraw_funcs_count; i++) {

    if (okadraw_funcs[i].active) {
      okadraw_funcs[i].draw(okadraw_funcs[i].arg, ws2812_buffer, mtime);
    }
  }

  if (okadraw_low_power_enabled) {
    for (i = 0; i < (STRIP_LENGTH * 3) - 1; i++) {
      ws2812_buffer[i] >>= 3;
    }
  }

  WS2812Refresh();
}

void ICACHE_FLASH_ATTR okadraw_set_low_power(bool state)
{
  okadraw_low_power_enabled = state;
}

////////////////////////////////////////////////////////////////////////////////

int32 okadraw_easing_linear(int32 out_max, int32 in_max, int32 in_val)
{
  return in_val * out_max / in_max;
}
int32 okadraw_easing_linear_back(int32 out_max, int32 in_max, int32 in_val)
{
  /* os_printf("okadraw_easing_linear_back %d, %d, %d\n", out_max, in_max, in_val); */

  int32 val = in_val * 2 * out_max / in_max;
  /* os_printf("val1 = %d\n", val); */
  if (val > out_max) {
    val = out_max * 2 - val;
  }
  /* os_printf("val2 = %d\n", val); */
  return val;
}

static void _draw_plain(oka_draw_plain_arg_t *arg, char *strip)
{
  okadraw_draw_hsv_row(strip, 0, STRIP_LENGTH - 1, arg->color);
}


void okadraw_draw_hsv_gradient(okadraw_hsv_gradient_t *arg, okadraw_strip_ptr strip, uint32 mtime)
{
  okadraw_idx_t idx;
  HsvColor color = arg->color1;

  for (idx = arg->start_idx; idx <= arg->end_idx; idx++) {
    if (arg->start_idx != arg->end_idx) {
      color.h = OKA_LINEAR_PROP(arg->color1.h, arg->color2.h, idx, arg->start_idx, arg->end_idx);
      color.s = OKA_LINEAR_PROP(arg->color1.s, arg->color2.s, idx, arg->start_idx, arg->end_idx);
      color.v = OKA_LINEAR_PROP(arg->color1.v, arg->color2.v, idx, arg->start_idx, arg->end_idx);
    }

    okadraw_draw_hsv(strip, idx, color);
    if (arg->mirror) {
      okadraw_draw_hsv(strip, STRIP_LENGTH  - idx - 1, color);
    }
  }
}

void okadraw_draw_rgb_gradient(okadraw_rgb_gradient_t *arg, okadraw_strip_ptr strip, uint32 mtime)
{
  okadraw_idx_t idx;
  RgbColor color;

  for (idx = arg->start_idx; idx <= arg->end_idx; idx++) {
    color.r = OKA_LINEAR_PROP(arg->color1.r, arg->color2.r, idx, arg->start_idx, arg->end_idx);
    color.g = OKA_LINEAR_PROP(arg->color1.g, arg->color2.g, idx, arg->start_idx, arg->end_idx);
    color.b = OKA_LINEAR_PROP(arg->color1.b, arg->color2.b, idx, arg->start_idx, arg->end_idx);

    if (arg->mirror) {
      OKADRAW_SET_RGB(strip, STRIP_LENGTH  - idx - 1, color);
    }
  }
}

void okadraw_draw_time_fx(okadraw_time_fx_t *arg, okadraw_strip_ptr strip, uint32 mtime)
{
  if (mtime > arg->start_time + arg->duration) {
    if (arg->loops > 0) {
      arg->loops--;
      if (!arg->loops) {
        okadraw_remove((okadraw_func_draw_t) okadraw_draw_time_fx, arg);
      }
    }
    arg->start_time = mtime;
  }

  int32 value = arg->start_value +
    (arg->easing ? arg->easing : okadraw_easing_linear)
    (arg->end_value - arg->start_value, arg->duration, mtime - arg->start_time);

  switch (arg->ptr_type) {
  case OKADRAW_PROP_INT8: *(int8 *)arg->ptr = value; break;
  case OKADRAW_PROP_INT16: *(int16_t *)arg->ptr = value; break;
  case OKADRAW_PROP_INT32: *(int32 *)arg->ptr = value; break;
  }
}

////////////////////////////////////////////////////////////////////////////////

oka_draw_plain_arg_t okadraw_bg = {
  .color = { h: 20, s: 255, v: /*60*/ 0 },
  .opacity = 100
};

static volatile os_timer_t autorefresh_timer;

void autorefresh_timerfunc(void *arg)
{
  okadraw_redraw();
}

void WS2812Init()
{
  okadraw_funcs_count = 0;
  okadraw_add((okadraw_func_draw_t)&_draw_plain, &okadraw_bg);

  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);
  //Set GPIO4 low
  gpio_output_set(0, BIT4, BIT4, 0);

  bzero(ws2812_buffer, WS2812_BUFFER_SIZE);
  WS2812Refresh();
  os_timer_disarm(&autorefresh_timer);
  os_timer_setfn(&autorefresh_timer, (os_timer_func_t *)autorefresh_timerfunc, NULL);
  os_timer_arm(&autorefresh_timer, 30, 1);
}

void WS2812Refresh()
{
  WS2812OutBuffer(ws2812_buffer, WS2812_BUFFER_SIZE);
}

uint16_t panel_distance(uint16_t px1, uint16_t px2)
{
  uint16_t distance = abs(px1 - px2);

  if (distance >= PANEL_MAX_DISTANCE) {
    distance = PANEL_SIZE - distance;
  }

  return distance;
}

////////////////////////////////////////////////////////////////////////////////

void okadraw_draw_hsv(okadraw_strip_ptr strip, okadraw_idx_t idx, HsvColor color)
{
  RgbColor rgb = HsvToRgb(color);

  OKADRAW_SET_RGB(strip, idx, rgb);
}

void okadraw_draw_hsv_row(okadraw_strip_ptr strip, okadraw_idx_t idx1, okadraw_idx_t idx2, HsvColor color)
{
  int i;

  for (i = idx1; i < idx2; i++) {
    okadraw_draw_hsv(strip, i, color);
  }
}

////////////////////////////////////////////////////////////////////////////////

void WS2812SetHSV(okadraw_idx_t idx, unsigned char h, unsigned char s, unsigned char v)
{
  HsvColor hsv = { h: h, s: s, v: v };
  RgbColor rgb = HsvToRgb(hsv);

  WS2812_SET_RGB(idx, rgb.r, rgb.g, rgb.b);
}

void WS2812SetRow(okadraw_idx_t idx1, okadraw_idx_t idx2, unsigned char h)
{
  int i;

  for (i = idx1; i < idx2; i++) {
    WS2812SetHSV(i, h, 255, 255);
  }
}

void WS2812SetRowHsv(okadraw_idx_t idx1, okadraw_idx_t idx2,
                     unsigned char hue, unsigned char saturation, unsigned char value)
{
  int i;

  for (i = idx1; i < idx2; i++) {
    WS2812SetHSV(i, hue, saturation, value);
  }
}

void WS2812FadeRowValue(okadraw_idx_t idx1, okadraw_idx_t idx2, unsigned char hue, unsigned char saturation,
                        unsigned char value1, unsigned char value2)
{
  int i;

  for (i = idx1; i < idx2; i++) {
    WS2812SetHSV(i, hue, saturation, value1 + (value2 - value1) * i / (idx2 - idx1));
  }
}

void ICACHE_FLASH_ATTR WS2812MirrorPixels(uint16_t start, size_t length)
{
  memcpy(ws2812_buffer + (WS2812_BUFFER_SIZE - start - 1) * 3, ws2812_buffer + start * 3, length * 3);
}


RgbColor HsvToRgb(HsvColor hsv)
{
  RgbColor rgb;
  unsigned char region, remainder, p, q, t;

  if (hsv.s == 0) {
    rgb.r = hsv.v;
    rgb.g = hsv.v;
    rgb.b = hsv.v;
    return rgb;
  }

  region = hsv.h / 43;
  remainder = (hsv.h - (region * 43)) * 6;

  p = (hsv.v * (255 - hsv.s)) >> 8;
  q = (hsv.v * (255 - ((hsv.s * remainder) >> 8))) >> 8;
  t = (hsv.v * (255 - ((hsv.s * (255 - remainder)) >> 8))) >> 8;

  switch (region) {
  case 0:
    rgb.r = hsv.v; rgb.g = t; rgb.b = p;
    break;
  case 1:
    rgb.r = q; rgb.g = hsv.v; rgb.b = p;
    break;
  case 2:
    rgb.r = p; rgb.g = hsv.v; rgb.b = t;
    break;
  case 3:
    rgb.r = p; rgb.g = q; rgb.b = hsv.v;
    break;
  case 4:
    rgb.r = t; rgb.g = p; rgb.b = hsv.v;
    break;
  default:
    rgb.r = hsv.v; rgb.g = p; rgb.b = q;
    break;
  }

  return rgb;
}

HsvColor RgbToHsv(RgbColor rgb)
{
  HsvColor hsv;
  unsigned char rgbMin, rgbMax;

  rgbMin = rgb.r < rgb.g ? (rgb.r < rgb.b ? rgb.r : rgb.b) : (rgb.g < rgb.b ? rgb.g : rgb.b);
  rgbMax = rgb.r > rgb.g ? (rgb.r > rgb.b ? rgb.r : rgb.b) : (rgb.g > rgb.b ? rgb.g : rgb.b);

  hsv.v = rgbMax;
  if (hsv.v == 0)
    {
      hsv.h = 0;
      hsv.s = 0;
      return hsv;
    }

  hsv.s = 255 * (long)(rgbMax - rgbMin) / hsv.v;
  if (hsv.s == 0)
    {
      hsv.h = 0;
      return hsv;
    }

  if (rgbMax == rgb.r)
    hsv.h = 0 + 43 * (rgb.g - rgb.b) / (rgbMax - rgbMin);
  else if (rgbMax == rgb.g)
    hsv.h = 85 + 43 * (rgb.b - rgb.r) / (rgbMax - rgbMin);
  else
    hsv.h = 171 + 43 * (rgb.r - rgb.g) / (rgbMax - rgbMin);

  return hsv;
}

/* void test_hsv(unsigned char h, unsigned char s, unsigned char v) */
/* { */
/*   RgbColor rgb; */
/*   HsvColor hsv = { h: h, s: s, v: v }; */

/*   rgb = HsvToRgb(hsv); */
/*   os_printf("HSV(%d, %d, %d) = RGB(%d, %d, %d)\n", hsv.h, hsv.s, hsv.v, rgb.r, rgb.g, rgb.b); */
/* } */

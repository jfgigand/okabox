#ifndef _WS2812_H
#define _WS2812_H

#include "c_types.h"
#include "os_type.h"
#include "user_interface.h"
#include "ets_sys.h"
#include "gpio.h"

#define WSGPIO 4

#define PANEL_SIZE              60
#define STRIP_LENGTH            PANEL_SIZE * 2
#define WS2812_BUFFER_SIZE      STRIP_LENGTH * 3
#define ABS(n)                    ((n) >= 0 ? (n) : -(n))
#define PANEL_MAX_DISTANCE        (PANEL_SIZE / 2)
/* #define PANEL_DISTANCE(px1, px2)  (ABS(((px1) % PANEL_SIZE) - ((px2) % PANEL_SIZE)) < PANEL_MAX_DISTANCE ? \ */
/*                                    ABS(((px1) % PANEL_SIZE) - ((px2) % PANEL_SIZE)) : \ */
/*                                    PANEL_SIZE - ABS(((px1) % PANEL_SIZE) - ((px2) % PANEL_SIZE))) */
/* #define PANEL_DISTANCE(px1, px2)  (ABS(px1 - px2) < PANEL_MAX_DISTANCE ? \ */
/*                                    ABS(px1 - px2) : PANEL_SIZE - ABS(px1 - px2)) */

#define WS2812_SET_RGB(idx, r, g, b)  \
  ws2812_buffer[(idx) * 3 + 0] = g; \
  ws2812_buffer[(idx) * 3 + 1] = r; \
  ws2812_buffer[(idx) * 3 + 2] = b;

#define WS2812_SET_RGB_MIRROR(idx, r, g, b) \
  WS2812_SET_RGB(idx, r, g, b); \
  WS2812_SET_RGB(STRIP_LENGTH - (idx) - 1, r, g, b);

typedef uint16_t okadraw_idx_t;
typedef char * okadraw_strip_ptr;

typedef struct RgbColor
{
  unsigned char r;
  unsigned char g;
  unsigned char b;
} RgbColor;

typedef struct HsvColor
{
  unsigned char h;
  unsigned char s;
  unsigned char v;
} HsvColor;


/* extern grbpx strip[STRIP_LENGTH]; */
extern char    ws2812_buffer[WS2812_BUFFER_SIZE];

/* #define MATRIX_GRB(strip, width, col, row) ((grbpx *)strip + (row * width + col)]) */
/* int ICACHE_FLASH_ATTR ws2812_writegrb(uint8_t pin, const grbpx *strip, size_t length); */

RgbColor HsvToRgb(HsvColor hsv);
HsvColor RgbToHsv(RgbColor rgb);

//You will have to 	os_intr_lock();  	os_intr_unlock();
void                    WS2812Init();
void ICACHE_FLASH_ATTR  WS2812OutBuffer( const char *, size_t length );
void ICACHE_FLASH_ATTR  WS2812Refresh();
void WS2812SetRow(okadraw_idx_t idx1, okadraw_idx_t idx2, unsigned char h);
void ICACHE_FLASH_ATTR  WS2812SetHSV(okadraw_idx_t idx, uint8_t h, uint8_t s, uint8_t v);
void ICACHE_FLASH_ATTR  WS2812MirrorPixels(uint16_t start, size_t length);
void                    WS2812SetRow(okadraw_idx_t idx1, okadraw_idx_t idx2, unsigned char h);
void                    WS2812SetRowHSV(okadraw_idx_t idx, uint8_t h, uint8_t s, uint8_t v);
void                    WS2812FadeRowValue(okadraw_idx_t idx1, okadraw_idx_t idx2,
                                           unsigned char hue, unsigned char saturation,
                                           unsigned char value1, unsigned char value2);

#define OKADRAW_SET_RGB(strip, idx, rgb_color)  \
  strip[(idx) * 3 + 0] = rgb_color.g;           \
  strip[(idx) * 3 + 1] = rgb_color.r;           \
  strip[(idx) * 3 + 2] = rgb_color.b;

#define OKADRAW_AVG_RGB(strip, idx, rgb_color)                          \
  strip[(idx) * 3 + 0] = (rgb_color.g + strip[(idx) * 3 + 0]) / 2;      \
  strip[(idx) * 3 + 1] = (rgb_color.r + strip[(idx) * 3 + 1]) / 2;      \
  strip[(idx) * 3 + 2] = (rgb_color.b + strip[(idx) * 3 + 2]) / 2;

#define HSV_COLOR(_h, _s, _v)      (HsvColor){ h: _h, s: _s, v: _v }

#define OKA_LINEAR_PROP(val1, val2, cur, cur_min, cur_max)      \
  (val1) + ((val2) - (val1)) * ((cur) - (cur_min)) / ((cur_max) - (cur_min))

typedef void   (*okadraw_func_draw_t)(void *arg, char *strip, uint32 time);
typedef int32 (*okadraw_easing_t)(int32 out_max, int32 in_max, int32 in_val);

typedef struct okadraw_func
{
  bool                  active;
  okadraw_func_draw_t   draw;
  void                  *arg;
} okadraw_func_t;

////////////////////////////////////////////////////////////////////////////////

typedef struct
{
  HsvColor      color;
  uint8         opacity;
} oka_draw_plain_arg_t;

typedef struct {
  HsvColor      color1;
  HsvColor      color2;
  okadraw_idx_t start_idx;
  okadraw_idx_t end_idx;
  bool          mirror;

} okadraw_hsv_gradient_t;

typedef struct {
  RgbColor      color1;
  RgbColor      color2;
  okadraw_idx_t start_idx;
  okadraw_idx_t end_idx;
  bool          mirror;

} okadraw_rgb_gradient_t;


enum okadraw_prop_type {
  OKADRAW_PROP_INT8,
  OKADRAW_PROP_INT16,
  OKADRAW_PROP_INT32,
  OKADRAW_PROP_RGB_COLOR,
  OKADRAW_PROP_HSV_COLOR,
};

typedef struct {
  uint32                start_time;     // System time, to be initialized with 0
  uint32                loops;          // Number of loops to make, 0 for infinite
  uint32                duration;       // FX duration in milliseconds
  okadraw_easing_t      easing;

  uint32                start_value;
  uint32                end_value;

  void                  *ptr;           // pointer to target value
  enum okadraw_prop_type   ptr_type;

} okadraw_time_fx_t;


extern oka_draw_plain_arg_t okadraw_bg;

void ICACHE_FLASH_ATTR okadraw_set_low_power(bool state);
void okadraw_draw_hsv(okadraw_strip_ptr strip, okadraw_idx_t idx, HsvColor color);
void okadraw_draw_hsv_row(okadraw_strip_ptr strip, okadraw_idx_t idx1, okadraw_idx_t idx2, HsvColor color);

void okadraw_draw_hsv_gradient(okadraw_hsv_gradient_t *arg, okadraw_strip_ptr strip, uint32 time);
void okadraw_draw_rgb_gradient(okadraw_rgb_gradient_t *arg, okadraw_strip_ptr strip, uint32 time);
void okadraw_draw_time_fx(okadraw_time_fx_t *arg, okadraw_strip_ptr strip, uint32 time);

int32 okadraw_easing_linear_back(int32 out_max, int32 in_max, int32 in_val);
int32 okadraw_easing_linear(int32 out_max, int32 in_max, int32 in_val);



/* uint16_t                panel_distance(uint16_t px1, uint16_t px2); */
/* void                    panel_set(uint16_t index, grbpx px); */


// https://en.wikipedia.org/wiki/HSL_and_HSV
// http://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both


#endif

#ifndef _ANIM_H_
#define _ANIM_H_

#include "ets_sys.h"
#include "ws2812.h"

/* typedef struct { */
/*   HsvColor color1; */
/*   HsvColor color2; */

/*   bool h_is_time; */
/*   bool s_is_time; */
/*   bool v_is_time; */

/*   okadraw_idx_t start_idx; */
/*   okadraw_idx_t end_idx; */

/*   uint32 start_time;    // system time */
/*   uint32 loops; */
/*   uint32 duration;      // milliseconds */

/* } okanim_prop_t; */

/* extern okanim_prop_t okanim_fx_random; */



extern okadraw_hsv_gradient_t okanim_fx_random;

void ICACHE_FLASH_ATTR oka_anim_start();
void ICACHE_FLASH_ATTR oka_anim_random();
void ICACHE_FLASH_ATTR oka_anim_samsung_sensors(double *vals, size_t nb_vals);
void ICACHE_FLASH_ATTR oka_anim_set_bg(uint8 bg);
void ICACHE_FLASH_ATTR oka_anim_set_bg_hsv(uint8 h, uint8 s, uint8 v);
void ICACHE_FLASH_ATTR okanim_update_temp(int16_t temperature);
void ICACHE_FLASH_ATTR okanim_update_humidity(int16_t humidity);

#endif  // _ANIM_H_

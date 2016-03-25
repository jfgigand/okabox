#ifndef _OKATIME_H_
#define _OKATIME_H_

#include "time.h"
#include "ets_sys.h"


struct tm *     ICACHE_FLASH_ATTR oka_time_gmtime_r(const time_t *tim_p, struct tm *res);
struct tm *     ICACHE_FLASH_ATTR oka_time_gmtime(const time_t * tim_p);
struct tm *     ICACHE_FLASH_ATTR oka_time_get_tm();
void            ICACHE_FLASH_ATTR oka_time_init();
const char *    ICACHE_FLASH_ATTR oka_time_format();
void            ICACHE_FLASH_ATTR oka_time_update(time_t *timestamp);
void            ICACHE_FLASH_ATTR oka_time_sync();


#endif // _OKATIME_H_

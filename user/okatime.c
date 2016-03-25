/**
 * okatime.c -- time management for okabox
 *
 * see also functions: sntp_get_timezone(), sntp_get_real_time(ts), sntp_localtime()
 *
 * gmtime() is from new_lib: https://sourceware.org/git/gitweb.cgi?p=newlib-cygwin.git;a=blob;f=newlib/libc/time/gmtime_r.c;h=81c7c94b1f9fe54f83c677e15c2ef6cc81509445;hb=HEAD
 *
 */

/**
 * gmtime_r.c
 * Original Author: Adapted from tzcode maintained by Arthur David Olson.
 * Modifications:
 * - Changed to mktm_r and added __tzcalc_limits - 04/10/02, Jeff Johnston
 * - Fixed bug in mday computations - 08/12/04, Alex Mogilnikov <alx@intellectronika.ru>
 * - Fixed bug in __tzcalc_limits - 08/12/04, Alex Mogilnikov <alx@intellectronika.ru>
 * - Move code from _mktm_r() to gmtime_r() - 05/09/14, Freddie Chopin <freddie_chopin@op.pl>
 * - Fixed bug in calculations for dates after year 2069 or before year 1901. Ideas for
 *   solution taken from musl's __secs_to_tm() - 07/12/2014, Freddie Chopin
 *   <freddie_chopin@op.pl>
 * - Use faster algorithm from civil_from_days() by Howard Hinnant - 12/06/2014,
 * Freddie Chopin <freddie_chopin@op.pl>
 *
 * Converts the calendar time pointed to by tim_p into a broken-down time
 * expressed as local time. Returns a pointer to a structure containing the
 * broken-down time.
 */

// Start "local.h"
/* local header used by libc/time routines */
/* #include <_ansi.h> */
#include <time.h>
#include "okatime.h"
#include "osapi.h"
#include "os_type.h"
#include "ip_addr.h"

#define SECSPERMIN	60L
#define MINSPERHOUR	60L
#define HOURSPERDAY	24L
#define SECSPERHOUR	(SECSPERMIN * MINSPERHOUR)
#define SECSPERDAY	(SECSPERHOUR * HOURSPERDAY)
#define DAYSPERWEEK	7
#define MONSPERYEAR	12

#define YEAR_BASE	1900
#define EPOCH_YEAR      1970
#define EPOCH_WDAY      4
#define EPOCH_YEARS_SINCE_LEAP 2
#define EPOCH_YEARS_SINCE_CENTURY 70
#define EPOCH_YEARS_SINCE_LEAP_CENTURY 370

#define isleap(y) ((((y) % 4) == 0 && ((y) % 100) != 0) || ((y) % 400) == 0)

/* int         _EXFUN (__tzcalc_limits, (int __year)); */

extern _CONST int __month_lengths[2][MONSPERYEAR];

/* _VOID _EXFUN(_tzset_unlocked_r, (struct _reent *)); */
/* _VOID _EXFUN(_tzset_unlocked, (_VOID)); */

/* /\* locks for multi-threading *\/ */
/* #ifdef __SINGLE_THREAD__ */
/* #define TZ_LOCK */
/* #define TZ_UNLOCK */
/* #else */
/* #define TZ_LOCK __tz_lock() */
/* #define TZ_UNLOCK __tz_unlock() */
/* #endif */

/* void _EXFUN(__tz_lock,(_VOID)); */
/* void _EXFUN(__tz_unlock,(_VOID)); */
// End "local.h"

/* Move epoch from 01.01.1970 to 01.03.0000 (yes, Year 0) - this is the first
 * day of a 400-year long "era", right after additional day of leap year.
 * This adjustment is required only for date calculation, so instead of
 * modifying time_t value (which would require 64-bit operations to work
 * correctly) it's enough to adjust the calculated number of days since epoch.
 */
#define EPOCH_ADJUSTMENT_DAYS	719468L
/* year to which the adjustment was made */
#define ADJUSTED_EPOCH_YEAR	0
/* 1st March of year 0 is Wednesday */
#define ADJUSTED_EPOCH_WDAY	3
/* there are 97 leap years in 400-year periods. ((400 - 97) * 365 + 97 * 366) */
#define DAYS_PER_ERA		146097L
/* there are 24 leap years in 100-year periods. ((100 - 24) * 365 + 24 * 366) */
#define DAYS_PER_CENTURY	36524L
/* there is one leap year every 4 years */
#define DAYS_PER_4_YEARS	(3 * 365 + 366)
/* number of days in a non-leap year */
#define DAYS_PER_YEAR		365
/* number of days in January */
#define DAYS_IN_JANUARY		31
/* number of days in non-leap February */
#define DAYS_IN_FEBRUARY	28
/* number of years per era */
#define YEARS_PER_ERA		400


static struct tm _reent_tm;

static time_t oka_last_timestamp = 0;
static uint32 oka_last_system_time = 0;


struct tm *
ICACHE_FLASH_ATTR
oka_time_gmtime_r(const time_t *tim_p, struct tm *res)
{
  long days, rem;
  _CONST time_t lcltime = *tim_p;
  int era, weekday, year;
  unsigned erayear, yearday, month, day;
  unsigned long eraday;

  days = ((long)lcltime) / SECSPERDAY + EPOCH_ADJUSTMENT_DAYS;
  rem = ((long)lcltime) % SECSPERDAY;
  if (rem < 0)
    {
      rem += SECSPERDAY;
      --days;
    }

  /* compute hour, min, and sec */
  res->tm_hour = (int) (rem / SECSPERHOUR);
  rem %= SECSPERHOUR;
  res->tm_min = (int) (rem / SECSPERMIN);
  res->tm_sec = (int) (rem % SECSPERMIN);

  /* compute day of week */
  if ((weekday = ((ADJUSTED_EPOCH_WDAY + days) % DAYSPERWEEK)) < 0)
    weekday += DAYSPERWEEK;
  res->tm_wday = weekday;

  /* compute year, month, day & day of year */
  /* for description of this algorithm see
   * http://howardhinnant.github.io/date_algorithms.html#civil_from_days */
  era = (days >= 0 ? days : days - (DAYS_PER_ERA - 1)) / DAYS_PER_ERA;
  eraday = days - era * DAYS_PER_ERA;	/* [0, 146096] */
  erayear = (eraday - eraday / (DAYS_PER_4_YEARS - 1) + eraday / DAYS_PER_CENTURY -
             eraday / (DAYS_PER_ERA - 1)) / 365;	/* [0, 399] */
  yearday = eraday - (DAYS_PER_YEAR * erayear + erayear / 4 - erayear / 100);	/* [0, 365] */
  month = (5 * yearday + 2) / 153;	/* [0, 11] */
  day = yearday - (153 * month + 2) / 5 + 1;	/* [1, 31] */
  month += month < 10 ? 2 : -10;
  year = ADJUSTED_EPOCH_YEAR + erayear + era * YEARS_PER_ERA + (month <= 1);

  res->tm_yday = yearday >= DAYS_PER_YEAR - DAYS_IN_JANUARY - DAYS_IN_FEBRUARY ?
    yearday - (DAYS_PER_YEAR - DAYS_IN_JANUARY - DAYS_IN_FEBRUARY) :
    yearday + DAYS_IN_JANUARY + DAYS_IN_FEBRUARY + isleap(erayear);
  res->tm_year = year - YEAR_BASE;
  res->tm_mon = month;
  res->tm_mday = day;

  res->tm_isdst = 0;

  return (res);
}

struct tm *
ICACHE_FLASH_ATTR
oka_time_gmtime(const time_t * tim_p)
{
  return oka_time_gmtime_r(tim_p, &_reent_tm);
}

time_t
ICACHE_FLASH_ATTR
oka_time_get()
{
  uint32 system_time = system_get_time();

  return oka_last_timestamp + (system_time - oka_last_system_time) / 1000000;
}

struct tm *
ICACHE_FLASH_ATTR
oka_time_get_tm()
{
  time_t time = oka_time_get();

  return oka_time_gmtime(&time);
}

static char _time_format_reent[64];

const char * ICACHE_FLASH_ATTR
oka_time_format()
{
  struct tm *time = oka_time_get_tm();

  os_sprintf(_time_format_reent, "%4u-%2u-%2u %2u:%2u:%2u",
             time->tm_year, time->tm_mon, time->tm_mday,
             time->tm_hour, time->tm_min, time->tm_sec);

  return _time_format_reent;
}

static volatile os_timer_t _init_timer;

void ICACHE_FLASH_ATTR
_init_timerfunc()
{
  time_t ts = sntp_get_current_timestamp();

  if (ts > 0) {
    oka_time_update(&ts);
  }

  if (oka_last_timestamp > 0) {
    os_timer_disarm(&_init_timer);
  } else {
    os_printf("Waiting for clock...\n");
  }
}

void ICACHE_FLASH_ATTR
oka_time_init()
{
  // see also: https://github.com/raburton/esp8266/tree/master/ntp
  ip_addr_t ntp_server;
  IP4_ADDR(&ntp_server, 192, 168, 42, 254);
  sntp_setserver(0, &ntp_server);
  sntp_init();
  sntp_set_timezone(1);
  os_printf("Initialized SNTP.\n");

  os_timer_disarm(&_init_timer);
  os_timer_setfn(&_init_timer, (os_timer_func_t *)_init_timerfunc, NULL);
  os_timer_arm(&_init_timer, 5000, 1);

}

void ICACHE_FLASH_ATTR
oka_time_update(time_t *timestamp)
{
  oka_last_system_time = system_get_time();
  oka_last_timestamp = *timestamp;
  os_printf("OKA Clock time updated to: %s [system = %d]\n",
            oka_time_format(), oka_last_system_time);
}

void ICACHE_FLASH_ATTR
oka_time_sync()
{
  sntp_stop();
  sntp_init();

  time_t ts = sntp_get_current_timestamp();

  if (ts > 0) {
    oka_time_update(&ts);
  }
}

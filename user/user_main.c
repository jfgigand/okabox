#include "user_interface.h"
#include "ets_sys.h"
#include "osapi.h"
/* #include "gpio.h" */
#include "os_type.h"
#include "c_types.h"
/* #include "ip_addr.h" */
#include "espconn.h"
#include "fastmath.h"
#include "sntp.h"
#include "user_config.h"
#include "cmd.h"
#include "network.h"
#include "ws2812.h"
#include "okatime.h"
#include "okardware.h"
#include "anim.h"

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1


os_event_t    user_procTaskQueue[user_procTaskQueueLen];
static void user_procTask(os_event_t *events);

static volatile os_timer_t user_timer;


struct tm *my_gmtime(const time_t * tim_p);

void ICACHE_FLASH_ATTR user_timerfunc(void *arg)
{
}

//Do nothing function
static void ICACHE_FLASH_ATTR user_procTask(os_event_t *events)
{
  os_printf("what\n");
  os_delay_us(10);
}

/* void my_test() */
/* { */
/*   os_printf("heyo %d, %d, %d, %d -- %d, %d\n", 300, 300 % 256, 256 % 256, 300 / 256, 512 % 256, 512 / 256); */
/* } */

//Init function
void ICACHE_FLASH_ATTR
user_init()
{
  oka_cmd_console_init(OKA_LEVEL_INFO);
  okardware_init();
  /* my_test(); */

  oka_wifi_init();

  //Set GPIO2 to output mode
  /* PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2); */

  WS2812Init();
  oka_anim_start();

  oka_udp_init();

  /* system_timer_reinit(); */

  oka_time_init();

  os_timer_disarm(&user_timer);
  os_timer_setfn(&user_timer, (os_timer_func_t *)user_timerfunc, NULL);
  os_timer_arm(&user_timer, 2000, 1);

  system_os_task(user_procTask, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);

  oka_cmd_shell_init();
  /* cos(2); */
}

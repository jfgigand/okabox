
#ifndef _NETWORK_H_
#define _NETWORK_H_

#include "espconn.h"

// echo ":test:0" | socat - UDP-DATAGRAM:255.255.255.255:5555,broadcast

extern struct espconn *oka_udp_collectd;

void oka_wifi_init();

void udpserver_recv(void *arg, char *pusrdata, unsigned short len);
void ICACHE_FLASH_ATTR oka_udp_send_str(const char *msg);

void ICACHE_FLASH_ATTR oka_udp_init();


#endif // _NETWORK_H_

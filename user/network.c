// echo hey | socat - UDP-DATAGRAM:255.255.255.255:5555,broadcast

#include "user_interface.h"
#include "osapi.h"
/* #include "stdlib.h" */
#include "mem.h"
#include "espconn.h"
#include "cmd.h"
#include "user_config.h"

static struct espconn *pUdpServer;
static struct espconn *oka_udp_broadcast;

void oka_wifi_init()
{
  struct station_config stationConf;

  wifi_status_led_install(OKA_WIFI_LED_IO_NUM, OKA_WIFI_LED_IO_MUX, OKA_WIFI_LED_IO_FUNC);

  /* int wifiMode = wifi_get_opmode(); */

  /* uint8 opmode: WiFi operating modes: */
  /*   0x01: station mode */
  /*   0x02: soft-AP mode */
  /*   0x03: station+soft-AP */
  wifi_set_opmode( 0x01 );

  ets_memcpy(&stationConf.ssid, SSID, ets_strlen( SSID ) + 1);
  ets_memcpy(&stationConf.password, PASSWORD, ets_strlen( PASSWORD ) + 1);

  wifi_station_set_config(&stationConf);
  wifi_station_connect();
}

float my_atof(const char* s){
  float rez = 0, fact = 1;
  int point_seen;

  if (*s == '-'){
    s++;
    fact = -1;
  };
  for (point_seen = 0; *s; s++){
    if (*s == '.'){
      point_seen = 1;
      continue;
    };
    int d = *s - '0';
    if (d >= 0 && d <= 9){
      if (point_seen) fact /= 10.0f;
      rez = rez * 10.0f + (float)d;
    };
  };
  return rez * fact;
};

/* double my_atof(const char* num) */
/* { */
/*   if (!num || !*num) */
/*     return 0; */
/*   double integerPart = 0; */
/*   double fractionPart = 0; */
/*   int divisorForFraction = 1; */
/*   int sign = 1; */
/*   bool inFraction = false; */
/*   /\*Take care of +/- sign*\/ */
/*   if (*num == '-') { */
/*     ++num; */
/*     sign = -1; */
/*   } */
/*   else if (*num == '+') { */
/*     ++num; */
/*   } */
/*   while (*num != '\0') { */
/*     if (*num >= '0' && *num <= '9') { */
/*       if (inFraction) { */
/*         /\*See how are we converting a character to integer*\/ */
/*         fractionPart = fractionPart*10 + (*num - '0'); */
/*         divisorForFraction *= 10; */
/*       } else { */
/*         integerPart = integerPart*10 + (*num - '0'); */
/*       } */
/*     } */
/*     else if (*num == '.') { */
/*       if (inFraction) */
/*         return sign * (integerPart + fractionPart/divisorForFraction); */
/*       else */
/*         inFraction = true; */
/*     } else { */
/*       return sign * (integerPart + fractionPart/divisorForFraction); */
/*     } */
/*     ++num; */
/*   } */
/*   return sign * (integerPart + fractionPart/divisorForFraction); */
/* } */

void ICACHE_FLASH_ATTR
process_samsung_sensors(char *pusrdata, unsigned short len)
{
  int i;
  int vi = 0, last = 0;
  double vals[13];
  char str[256];
  uint8_t ignore = 0;

  for (i = 0; i < len; i++) {
    if (i - last >= 256) {
      os_printf("UDP parse error: maximum field length (255) exceeded: ignoring line\n");
      ignore = 1;
      break;
    }
    if (',' == pusrdata[i]) {
      ets_memcpy(str, pusrdata + last, i - last);
      str[i - last] = '\0';
      vals[vi] = my_atof(str);
      /* os_printf("val %d = \"%s\"", vi, str); */
      last = i + 1;
      vi++;
      if (vi >= 13) {
        os_printf("UDP parse error: extra comma or value: ignoring line\n");
        ignore = 1;
        break;
      }
    }
  }

  if (!ignore) {
    /* os_printf(" \nVals[%d] = ", vi); */
    /* for (i = 0; i < vi; i++) { */
      /* os_printf("%f, ", vals[i]); */
      /* dtoa(vals[i]); */
    /* } */
    /* os_printf("\n"); */
  }
  oka_anim_samsung_sensors(vals, vi);

  //Seems to be optional, still can cause crashes.
  /* ets_wdt_disable(); */
  /* os_printf(buffer, "%03x", len); */

  /* WS2812OutBuffer( pusrdata, len ); */
  /* ets_sprintf( buffer, "%03x", len ); */
  /* uart0_sendStr(buffer); */
}

void ICACHE_FLASH_ATTR
oka_udp_send_str(char *msg)
{
  size_t len = os_strlen(msg);
  espconn_sendto(oka_udp_broadcast, msg, len + 1);
}

//Called when new packet comes in.
void ICACHE_FLASH_ATTR
udpserver_recv(void *arg, char *pusrdata, unsigned short len)
{
  int i;
  struct espconn *pespconn = (struct espconn *)arg;
  /* uint8_t buffer[MAX_FRAME]; */

  /* char *frame = os_zalloc(len + 1) */
  /* ets_memcpy */
  /* os_printf("UDP received [%d]: %.*s\n", len, len, pusrdata); */

  // DEBUG
  /* os_printf("UDP received %d bytes: ", len); */
  /* for (i = 0; i < len; i++) { */
  /*   os_printf( pusrdata[i] < ' ' || pusrdata[i] > '~' ? "<%x>" : "%c", pusrdata[i]); */
  /* } */
  /* os_printf("\n"); */

  if (':' == pusrdata[0]) {
    oka_cmd_eval(pusrdata + 1, len - 1);
  } else {
    process_samsung_sensors(pusrdata, len);
  }
}

void oka_udp_init()
{
  // Server:
  pUdpServer = (struct espconn *)os_zalloc(sizeof(struct espconn));
  ets_memset( pUdpServer, 0, sizeof( struct espconn ) );
  /* espconn_create( pUdpServer ); */
  pUdpServer->type = ESPCONN_UDP;
  pUdpServer->proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
  pUdpServer->proto.udp->local_port = UDP_PORT;
  espconn_regist_recvcb(pUdpServer, udpserver_recv);

  if (espconn_create(pUdpServer)) {
    os_printf("Failed to create UDP server.");
  } else {
    os_printf("UDP server listening on port %d\n", pUdpServer->proto.udp->local_port);
  }

  // Client:
  oka_udp_broadcast = (struct espconn *)os_zalloc(sizeof(struct espconn));
  ets_memset( oka_udp_broadcast, 0, sizeof( struct espconn ) );
  oka_udp_broadcast->type = ESPCONN_UDP;
  oka_udp_broadcast->proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
  oka_udp_broadcast->proto.udp->remote_port = UDP_PORT;
  struct ip_addr broadcast_ip;
  IP4_ADDR(&broadcast_ip, 255, 255, 255, 255);
  /* oka_udp_broadcast->proto.udp->remote_ip = ; */
  os_memcpy(oka_udp_broadcast->proto.udp->remote_ip, &broadcast_ip, 4);
  /* local_port=espconn_port() */
  /* espconn_regist_recvcb(oka_udp_broadcast, udpserver_recv); */

  if (espconn_create(oka_udp_broadcast)) {
    os_printf("Failed to create UDP server.");
  } else {
    os_printf("UDP server listening on port %d\n", oka_udp_broadcast->proto.udp->local_port);
  }

  oka_udp_send_str("Et me voilà !");
}

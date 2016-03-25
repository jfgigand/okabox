
#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_

/* #include "driver/key.h" */

// Network
#define SSID            "... set network name here ..."
#define PASSWORD        "... set password here ..."
#define UDP_PORT        5555
#define SERVER_TIMEOUT  1500
#define MAX_CONNS       5
#define MAX_FRAME       2000

//// From: SDK/include/eagle_soc.h
//
// 12   PERIPHS_IO_MUX_MTDI_U
// 13   PERIPHS_IO_MUX_MTCK_U
// 14   PERIPHS_IO_MUX_MTMS_U
// 15   PERIPHS_IO_MUX_MTDO_U

#define OKA_WIFI_LED_IO_NUM     14
#define OKA_WIFI_LED_IO_MUX     PERIPHS_IO_MUX_MTMS_U
#define OKA_WIFI_LED_IO_FUNC    FUNC_GPIO14

#define OKA_PCF8574_INT_IO_NUM  5
#define OKA_PCF8574_INT_IO_MUX  PERIPHS_IO_MUX_GPIO5_U
#define OKA_PCF8574_INT_IO_FUNC FUNC_GPIO5
/* #define OKA_PCF8574_INT_EDGE    GPIO_PIN_INTR_ANYEDGE */
#define OKA_PCF8574_INT_EDGE    GPIO_PIN_INTR_ANYEDGE

#define OKA_BUT1_IO_NUM         12
#define OKA_BUT1_IO_MUX         PERIPHS_IO_MUX_MTDI_U
#define OKA_BUT1_IO_FUNC        FUNC_GPIO12

#define OKA_DHT_IO_NUM          13
#define OKA_DHT_IO_MUX          PERIPHS_IO_MUX_MTCK_U
#define OKA_DHT_IO_FUNC         FUNC_GPIO13

#define OKA_SR501_IO_NUM        15
#define OKA_SR501_IO_MUX        PERIPHS_IO_MUX_MTDO_U
#define OKA_SR501_IO_FUNC       FUNC_GPIO15

#endif /* _USER_CONFIG_H_ */

#ifndef _OKARDWARE_H_
#define _OKARDWARE_H_

extern uint8 pcf8574_register;

void ICACHE_FLASH_ATTR okardware_init();
bool ICACHE_FLASH_ATTR okardware_check(uint8);


bool ICACHE_FLASH_ATTR okardware_i2c_select_addr(uint8 address, uint8 direction);
bool ICACHE_FLASH_ATTR pcf8574_set_direction(int8 direction);
bool ICACHE_FLASH_ATTR pcf8574_update();
void ICACHE_FLASH_ATTR pcf8574_set(uint8 value);
void ICACHE_FLASH_ATTR pcf8574_bit(uint8 bit, uint8 value);
uint8 ICACHE_FLASH_ATTR pcf8574_read();
void ICACHE_FLASH_ATTR okardware_select_adc(uint8 port);
uint16 ICACHE_FLASH_ATTR okardware_adc_read(uint8 port);
void ICACHE_FLASH_ATTR okardware_adc_print_all();
uint32 ICACHE_FLASH_ATTR okardware_read_temp();

#endif // _OKARDWARE_H_

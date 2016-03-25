// From: https://github.com/mathew-hall/esp8266-dht/blob/master/user/dht.h

enum sensor_type{
  SENSOR_DHT11,SENSOR_DHT22
};

struct sensor_reading{
  int16_t temperature;
  // between 0 and 1000 (pour mille)
  int16_t humidity;
  const char* source;
  uint8_t sensor_id[16];
  BOOL success;
};

extern void     (*dht_update_cb)(struct sensor_reading *reading);

void ICACHE_FLASH_ATTR DHT(void);
struct sensor_reading * ICACHE_FLASH_ATTR readDHT(int force);
void DHTInit(enum sensor_type, uint32_t polltime);

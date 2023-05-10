#ifndef _UTILS_H
#define _UTILS_H

void utils_setup();

void sleep();

char * utils_device_name();
unsigned char utils_get_battery();

uint8_t utils_get_bit(uint8_t * data, uint16_t position);
uint16_t utils_get_bits(uint8_t * data, uint16_t position, uint8_t len);
void utils_time_to_text(uint32_t time, char * buffer);
float utils_tps(uint32_t time, uint16_t turns);

void wdt_set(unsigned long seconds);
void wdt_feed();

#endif // _UTILS_H
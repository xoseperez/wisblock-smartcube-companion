#ifndef _UTILS_H
#define _UTILS_H

unsigned char utils_setup();

void sleep();

char * utils_device_name();
unsigned char utils_get_battery();

uint8_t utils_get_bit(uint8_t * data, uint16_t position);
uint16_t utils_get_bits(uint8_t * data, uint16_t position, uint8_t len);

void wdt_set(unsigned long seconds);
void wdt_feed();

#endif // _UTILS_H
#ifndef _UTILS_H
#define _UTILS_H

#include "config.h"

void utils_setup();
void utils_beep(unsigned short freq = BUZZER_FREQ_OK, unsigned long ms = BUZZER_DURATION);
void utils_delay(uint32_t ms);

void utils_sleep();
bool utils_sleeping();

char * utils_device_name();
unsigned char utils_get_battery();

uint8_t utils_get_bit(uint8_t * data, uint16_t position);
uint16_t utils_get_bits(uint8_t * data, uint16_t position, uint8_t len);
char * utils_time_to_text(uint32_t time, char * buffer, bool zeropad = false);
float utils_tps(uint32_t time, uint16_t turns);

void wdt_set(unsigned long seconds);
void wdt_feed();

#endif // _UTILS_H
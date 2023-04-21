#ifndef _UTILS_H
#define _UTILS_H

#define VBAT_MV_PER_LSB (0.73242188F) // 3.0V ADC range and 12-bit ADC resolution = 3000mV/4096
#define VBAT_DIVIDER (0.71275837F)    // 2M + 0.806M voltage divider on VBAT = (2M / (0.806M + 2M))
#define VBAT_DIVIDER_COMP (1.403F)    // Compensation factor for the VBAT divider
#define REAL_VBAT_MV_PER_LSB (VBAT_DIVIDER_COMP * VBAT_MV_PER_LSB)

void sleep();

char * utils_device_name();
void utils_set_peer_battery(uint8_t);
uint8_t utils_get_peer_battery();
void utils_set_peer_version(char * version);
char * utils_get_peer_version();

uint8_t utils_get_bit(uint8_t * data, uint16_t position);
uint16_t utils_get_bits(uint8_t * data, uint16_t position, uint8_t len);

void wdt_set(unsigned long seconds);
void wdt_feed();

#endif // _UTILS_H
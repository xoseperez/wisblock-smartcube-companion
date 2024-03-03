#ifndef _SYSTEM_H
#define _SYSTEM_H

void system_reset();
void system_off();
void system_sleep();
bool system_sleeping();

void system_wdt_enable(unsigned long seconds);
void system_wdt_disable();
void system_wdt_feed();
bool system_wdt_triggered();

void system_delay(uint32_t ms);

#endif // _SYSTEM_H
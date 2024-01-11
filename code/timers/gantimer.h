#ifndef _GANTIMER_H
#define _GANTIMER_H

void gantimer_init();
bool gantimer_start(uint16_t conn_handle);
void gantimer_stop();
unsigned char gantimer_state();
void gantimer_set_callback(void (*callback)(uint8_t event, uint8_t * data));

#endif // _GANTIMER_H

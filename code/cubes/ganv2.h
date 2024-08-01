#ifndef _GANV2_H
#define _GANV2_H

#define GANV2_GET_FACELETS 4
#define GANV2_GET_HARDWARE 5
#define GANV2_GET_BATTERY 9

void ganv2_init();
bool ganv2_start(uint16_t conn_handle);

// privates
void ganv2_data_send_raw(uint8_t* data, uint16_t len);

#endif // _GANV2_H
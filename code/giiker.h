#ifndef _GIIKER_H
#define _GIIKER_H

#define GIIKER_GET_FACELETS 4
#define GIIKER_GET_HARDWARE 5
#define GIIKER_GET_BATTERY 9

void giiker_init();
bool giiker_start(uint16_t conn_handle);

// privates
void giiker_data_send_raw(uint8_t* data, uint16_t len);

#endif // _GIIKER_H
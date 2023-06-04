#ifndef _GIIKER_H
#define _GIIKER_H

#define GIIKER_GET_BATTERY 0xB5
#define GIIKER_GET_FIRMWARE 0xB7

void giiker_init();
bool giiker_start(uint16_t conn_handle);

// privates
void giiker_data_send_raw(uint8_t* data, uint16_t len);

#endif // _GIIKER_H
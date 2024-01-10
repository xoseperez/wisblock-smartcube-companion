#ifndef _GOCUBE_H
#define _GOCUBE_H

#define GOCUBE_GET_BATTERY 50
#define GOCUBE_GET_STATE 51

void gocube_init();
bool gocube_start(uint16_t conn_handle);

// privates
void gocube_data_send(uint8_t opcode);
void gocube_data_send_raw(uint8_t* data, uint16_t len);

#endif // _GOCUBE_H
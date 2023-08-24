#ifndef _MOYUAI_H
#define _MOYUAI_H

void moyuai_init();
void moyuai_reset();
bool moyuai_start(uint16_t conn_handle);

// privates
void moyuai_data_send_raw(uint8_t* data, uint16_t len);

#endif // _MOYUAI_H
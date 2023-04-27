#ifndef _GAN_H
#define _GAN_H

#define GANV2_GET_FACELETS 4
#define GANV2_GET_HARDWARE 5
#define GANV2_GET_BATTERY 9

bool ganv2_start(uint16_t conn_handle);
void ganv2_stop();

// privates
void ganv2_data_send_raw(uint8_t* data, uint16_t len);

#endif // _GAN_H
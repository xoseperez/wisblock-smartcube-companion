#ifndef _GAN_H
#define _GAN_H

#define GAN_GET_FACELETS 4
#define GAN_GET_HARDWARE 5
#define GAN_GET_BATTERY 9

void gan_init();
bool gan_start(uint16_t conn_handle);

// privates
void gan_data_send_raw(uint8_t* data, uint16_t len);

#endif // _GAN_H
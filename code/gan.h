#ifndef _GAN_H
#define _GAN_H

#define GAN_GET_FACELETS 4
#define GAN_GET_HARDWARE 5
#define GAN_GET_BATTERY 9

bool gan_start(uint16_t conn_handle);
void gan_stop();
void gan_setup();

#endif // _GAN_H
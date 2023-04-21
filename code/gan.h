#ifndef _GAN_H
#define _GAN_H

// MAC cube: AB:12:34:5F:AE:82

#define GAN_GET_FACELETS 4
#define GAN_GET_HARDWARE 5
#define GAN_GET_BATTERY 9

bool gan_start(uint16_t conn_handle);
void gan_stop();

void gan_data_send(uint8_t* data, uint16_t len);
void gan_data_send(uint8_t opcode);

#endif // _GAN_H
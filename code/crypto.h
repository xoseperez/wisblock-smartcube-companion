#ifndef _CRYPTO_H
#define _CRYPTO_H

bool aes128_decode(uint8_t* data, uint16_t len);
bool aes128_encode(uint8_t * data, uint16_t len);
void aes128_init(uint8_t * key, uint8_t * iv);

#endif // _CRYPTO_H
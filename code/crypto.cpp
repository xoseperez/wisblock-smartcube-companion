#include <Arduino.h>
#include <Crypto.h>
#include <AES.h>

#include "config.h"
#include "crypto.h"

AESSmall128 _aes;

uint8_t _aes_key[16] = {0};
uint8_t _aes_iv[16] = {0};

bool aes128_decode(uint8_t* data, uint16_t len) {

    if (len > 16) {
        uint8_t offset = len - 16;
        _aes.decryptBlock(data + offset, data + offset);
        for (uint8_t i=0; i<16; i++) {
            data[i + offset] ^= _aes_iv[i];
        }
    }

    _aes.decryptBlock(data, data);
    for (uint8_t i=0; i<16; i++) {
        data[i] ^= _aes_iv[i];
    }
    
    return true;

}

bool aes128_encode(uint8_t * data, uint16_t len) {

    for (uint8_t i=0; i<16; i++) {
        data[i] ^= _aes_iv[i];
    }
    _aes.encryptBlock(data, data);

    if (len > 16) {
        uint8_t offset = len - 16;
        for (uint8_t i=0; i<16; i++) {
            data[i+offset] ^= _aes_iv[i];
        }
        _aes.encryptBlock(data + offset, data + offset);    
    }
    
    return true;

}

void aes128_init(uint8_t * key, uint8_t * iv) {
    
    // Copy keys
    memcpy(_aes_key, key, 16);
    memcpy(_aes_iv, iv, 16);

    // Set decryption key
    _aes.setKey(_aes_key, 16);

    #if DEBUG > 0
        Serial.printf("[AES] KEY: ");
        for (uint16_t i=0; i<16; i++) {
            Serial.printf("%02X", _aes_key[i]);
        }
        Serial.println();
        Serial.printf("[AES] IV : ");
        for (uint16_t i=0; i<16; i++) {
            Serial.printf("%02X", _aes_iv[i]);
        }
        Serial.println();
    #endif

}


#include <Arduino.h>
#include <Crypto.h>
#include <AES.h>
#include <bluefruit.h>

#include "config.h"
#include "bluetooth.h"
#include "gan.h"

AESSmall128 _gan_aes;

bool _gan_init = false;
uint8_t _gan_mac[8] = {0};
uint8_t _gan_key[16] = {0};
uint8_t _gan_iv[16] = {0};

const uint8_t GAN_UUID_SERVICE_V2_DATA[] = { 0x79, 0x41, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E };
const uint8_t GAN_UUID_CHARACTERISTIC_V2_READ[] = { 0xE4, 0xCC, 0xDB, 0xE2, 0x2A, 0x2A, 0x2F, 0xA3, 0xE9, 0x11, 0x67, 0xCD, 0xB6, 0x4C, 0xBE, 0x28 };
const uint8_t GAN_UUID_CHARACTERISTIC_V2_WRITE[] = { 0xE4, 0xCC, 0xDB, 0xE2, 0x2A, 0x2A, 0x2F, 0xA3, 0xE9, 0x11, 0x67, 0xCD, 0x4A, 0x4A, 0xBE, 0x28 };

uint8_t GAN_KEYS[4][16] = {
    {198,202, 21,223, 79,110, 19,182,119, 13,230, 89, 58,175,186,162},
    { 67,226, 91,214,125,220,120,216,  7, 96,163,218,130, 60,  1,241},
    {  1,  2, 66, 40, 49,145, 22,  7, 32,  5, 24, 84, 66, 17, 18, 83},
    { 17,  3, 50, 40, 33,  1,118, 39, 32,149,120, 20, 50, 18,  2, 67}
};

BLEClientService _gan_service_v2_data(GAN_UUID_SERVICE_V2_DATA);
BLEClientCharacteristic _gan_characteristic_v2_read(GAN_UUID_CHARACTERISTIC_V2_READ);
BLEClientCharacteristic _gan_characteristic_v2_write(GAN_UUID_CHARACTERISTIC_V2_WRITE);

bool gan_decode(uint8_t* data, uint16_t len) {

    if (len > 16) {
        uint8_t offset = len - 16;
        _gan_aes.decryptBlock(data + offset, data + offset);
        for (uint8_t i=0; i<16; i++) {
            data[i + offset] ^= _gan_iv[i];
        }
    }

    _gan_aes.decryptBlock(data, data);
    for (uint8_t i=0; i<16; i++) {
        data[i] ^= _gan_iv[i];
    }
    
    return true;

}

bool gan_encode(uint8_t * data, uint16_t len) {

    for (uint8_t i=0; i<16; i++) {
        data[i] ^= _gan_iv[i];
    }
    _gan_aes.encryptBlock(data, data);

    if (len > 16) {
        uint8_t offset = len - 16;
        for (uint8_t i=0; i<16; i++) {
            data[i+offset] ^= _gan_iv[i];
        }
        _gan_aes.encryptBlock(data + offset, data + offset);    
    }
    
    return true;

}

void gan_data_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len) {

    static int16_t count_old = -1;
    
    if (gan_decode(data, len)) {

        #if DEBUG > 1
            Serial.printf("[GAN] Received: ");
            for (uint16_t i=0; i<len; i++) {
                Serial.printf("%02X", data[i]);
            }
            Serial.println();
        #endif
        
        uint8_t mode = (data[0] & 0xF0) >> 4;
        uint8_t count = ((data[0] & 0xF) << 4) + ((data[1] & 0xF0) >> 4);
        
        if (1 == mode) { // Gyro
            // Nothing to do

        } else if (2 == mode) { // Cube move
            
            // Do not process if already processed
            if (count == count_old) return;
            count_old = count;

        } else if (4 == mode) { // Cube state

            // Do not process if already processed
            if (count == count_old) return;
            count_old = count;

        } else if (5 == mode) { // Hardware info
            
            char device[9] = {0};
            memcpy(device, &data[5], 8);
            bool gyro = (data[13] & 0x80) == 0x80;

            #if DEBUG > 0
                Serial.printf("[GAN] Hardware info message received\n");
                Serial.printf("[GAN] Device name     : %s\n", device);
                Serial.printf("[GAN] Hardware version: %d.%d\n", data[1], data[2]);
                Serial.printf("[GAN] Software version: %d.%d\n", data[3], data[4]);
                Serial.printf("[GAN] Gyro enabled    : %s\n", gyro ? "yes": "no");
            #endif

        } else if (9 == mode) { // Battery

            #if DEBUG > 0
                Serial.printf("[GAN] Battery         : %d%%\n", data[1]);
            #endif

        }

    }

}

void gan_data_send(uint8_t* data, uint16_t len) {

    #if DEBUG > 1
        Serial.printf("[GAN] Sending: ");
        for (uint16_t i=0; i<len; i++) {
            Serial.printf("%02X", data[i]);
        }
        Serial.println();
    #endif
    
    gan_encode(data, len);
    _gan_characteristic_v2_write.write(data, len);

}

void gan_data_send(uint8_t opcode) {
    uint8_t data[20];
    data[0] = opcode;
    gan_data_send(data, 20);
}

void gan_init_decoder(uint8_t * mac) {
    
    // Calculate keys
    memcpy(_gan_key, GAN_KEYS[2], 16);
    memcpy(_gan_iv, GAN_KEYS[3], 16);
    for (uint8_t i=0; i<6; i++) {
        _gan_key[i] = ( _gan_key[i] + mac[5-i] ) % 255;
        _gan_iv[i] = ( _gan_iv[i] + mac[5-i] ) % 255;
    }

    // Set decryption key
    _gan_aes.setKey(_gan_key, 16);

    #if DEBUG > 0
        Serial.printf("[GAN] KEY: ");
        for (uint16_t i=0; i<16; i++) {
            Serial.printf("%02X", _gan_key[i]);
        }
        Serial.println();
        Serial.printf("[GAN] IV : ");
        for (uint16_t i=0; i<16; i++) {
            Serial.printf("%02X", _gan_iv[i]);
        }
        Serial.println();
    #endif

}

void gan_get_mac(uint16_t conn_handle, uint8_t * mac) {

    // Get the reference to current connection
    BLEConnection* connection = Bluefruit.Connection(conn_handle);
    ble_gap_addr_t ble_addr = connection->getPeerAddr();
    mac[0] = ble_addr.addr[5];
    mac[1] = ble_addr.addr[4];
    mac[2] = ble_addr.addr[3];
    mac[3] = ble_addr.addr[2];
    mac[4] = ble_addr.addr[1];
    mac[5] = ble_addr.addr[0];

}

void gan_stop() {

}

bool gan_start(uint16_t conn_handle) {

     // Discover GAN v2 data service (only one supported right now)
   _gan_service_v2_data.begin();
    if ( !_gan_service_v2_data.discover(conn_handle) ) {
        #if DEBUG > 0
            Serial.println("[GAN] GAN v2 data service not found. Disconnecting.");
        #endif
        return false;
    }
    #if DEBUG > 0
        Serial.println("[GAN] GAN v2 data service found.");
    #endif

    gan_get_mac(conn_handle, _gan_mac);
    gan_init_decoder(_gan_mac);

    // Discover GAN v2 write characteristic
    _gan_characteristic_v2_write.begin();
    if ( ! _gan_characteristic_v2_write.discover() ) {
        #if DEBUG > 0
            Serial.println("[GAN] GAN v2 write characteristic not found. Disconnecting.");
        #endif
        return false;
    }
    #if DEBUG > 0
        Serial.println("[GAN] GAN v2 write characteristic found.");
    #endif

    // Discover GAN v2 read characteristic
    _gan_characteristic_v2_read.setNotifyCallback(gan_data_callback);
    _gan_characteristic_v2_read.begin();
    if ( ! _gan_characteristic_v2_read.discover() ) {
        #if DEBUG > 0
            Serial.println("[GAN] GAN v2 read characteristic not found. Disconnecting.");
        #endif
        return false;
    }
    _gan_characteristic_v2_read.enableNotify();
    #if DEBUG > 0
        Serial.println("[GAN] GAN v2 read characteristic found. Subscribed.");
    #endif

    // Query the cube
    gan_data_send(GAN_GET_HARDWARE);
    gan_data_send(GAN_GET_FACELETS);
    gan_data_send(GAN_GET_BATTERY);
    
    return true;
    
}

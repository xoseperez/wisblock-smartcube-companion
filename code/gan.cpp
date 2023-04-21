#include <Arduino.h>
#include <Crypto.h>
#include <AES.h>
#include <bluefruit.h>

#include "config.h"
#include "bluetooth.h"
#include "utils.h"
#include "gan.h"

AESSmall128 _gan_aes;

bool _gan_init = false;
uint8_t _gan_mac[8] = {0};
uint8_t _gan_key[16] = {0};
uint8_t _gan_iv[16] = {0};

uint32_t _gan_start = 0;
bool _gan_timer = 0;

const uint8_t GAN_UUID_SERVICE_V2_DATA[] = { 0x79, 0x41, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E };
const uint8_t GAN_UUID_CHARACTERISTIC_V2_READ[] = { 0xE4, 0xCC, 0xDB, 0xE2, 0x2A, 0x2A, 0x2F, 0xA3, 0xE9, 0x11, 0x67, 0xCD, 0xB6, 0x4C, 0xBE, 0x28 };
const uint8_t GAN_UUID_CHARACTERISTIC_V2_WRITE[] = { 0xE4, 0xCC, 0xDB, 0xE2, 0x2A, 0x2A, 0x2F, 0xA3, 0xE9, 0x11, 0x67, 0xCD, 0x4A, 0x4A, 0xBE, 0x28 };

uint8_t GAN_KEYS[4][16] = {
    {198,202, 21,223, 79,110, 19,182,119, 13,230, 89, 58,175,186,162},
    { 67,226, 91,214,125,220,120,216,  7, 96,163,218,130, 60,  1,241},
    {  1,  2, 66, 40, 49,145, 22,  7, 32,  5, 24, 84, 66, 17, 18, 83},
    { 17,  3, 50, 40, 33,  1,118, 39, 32,149,120, 20, 50, 18,  2, 67}
};
const char FACES[] = "URFDLB";
uint8_t SOLVED_CORNERS[] = {0, 1, 2, 3, 4, 5, 6, 7};
uint8_t SOLVED_EDGES[] = {0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22};
uint8_t CORNER_FACELET[8][3] = {
    {8, 9, 20}, // URF
    {6, 18, 38}, // UFL 
    {0, 36, 47}, // ULB
    {2, 45, 11}, // UBR
    {29, 26, 15}, // DFR
    {27, 44, 24}, // DLF
    {33, 53, 42}, // DBL
    {35, 17, 51}  // DRB
};
uint8_t EDGE_FACELET[12][2] = {
    {5, 10}, // UR
    {7, 19}, // UF
    {3, 37}, // UL
    {1, 46}, // UB
    {32, 16}, // DR
    {28, 25}, // DF
    {30, 43}, // DL
    {34, 52}, // DB
    {23, 12}, // FR
    {21, 41}, // FL
    {50, 39}, // BL
    {48, 14}  // BR
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

void gan_to_cube(uint8_t * corners, uint8_t * edges) {

    #if DEBUG>1
        Serial.printf("[GAN] Corners: ");
        for (uint16_t i=0; i<8; i++) {
            Serial.printf("%d ", corners[i]);
        }
        Serial.println();
        Serial.printf("[GAN] Edges: ");
        for (uint16_t i=0; i<12; i++) {
            Serial.printf("%d ", edges[i]);
        }
        Serial.println();
    #endif

    char facelets[55] = {0};
    for (uint8_t i=0; i<54; i++) {
        facelets[i] = FACES[int(i / 9)];
    }
    for (uint8_t c=0; c<8; c++) {
        uint8_t j = corners[c] & 0x07;
        uint8_t ori = corners[c] >> 3;
        for (uint8_t n=0; n<3; n++) {
            facelets[CORNER_FACELET[c][(n + ori) % 3]] = FACES[int(CORNER_FACELET[j][n] / 9)];
        }
    }
    for (uint8_t e=0; e<12; e++) {
        uint8_t j = edges[e] >> 1;
        uint8_t ori = edges[e] & 0x01;
        for (uint8_t n=0; n<2; n++) {
            facelets[EDGE_FACELET[e][(n + ori) % 2]] = FACES[int(EDGE_FACELET[j][n] / 9)];
        }
    }

    #if DEBUG>0
        Serial.printf("[GAN] State: %s\n", facelets);
    #endif

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

            uint16_t last4_sum = 0;
            for (uint8_t i=0; i<4; i++) {

                uint8_t turn = utils_get_bits(data, 12 + i*5, 5);
                last4_sum += turn;

                #if DEBUG > 0
                    if (i==0) {
                        Serial.print("[GAN] Movement: ");
                        Serial.print(FACES[turn >> 1]);
                        if (turn & 0x01) Serial.print("'");
                        Serial.println();
                    }
                #endif

            }
            
            if ((last4_sum == 0) & (!_gan_timer)) {
                #if DEBUG > 0
                    Serial.println("[GAN] 4 U turns in a row! Starting timer.");
                #endif
                _gan_timer = true;
                _gan_start = millis();
            }

        } else if (4 == mode) { // Cube state

            #if DEBUG>1
                Serial.printf("[GAN] Received: ");
                for (uint16_t i=0; i<len; i++) {
                    Serial.printf("%02X", data[i]);
                }
                Serial.println();
            #endif

            uint16_t echk = 0;
            uint16_t cchk = 0xf00;
            uint8_t cube_corners[8];
            uint8_t cube_edges[12];
            
            for (uint8_t i=0; i<7; i++) {
                uint8_t perm = utils_get_bits(data, 12 + i*3, 3);
                uint8_t ori = utils_get_bits(data, 33 + i*2, 2);
                cchk = (cchk - (ori << 3)) ^ perm;
                cube_corners[i] = (ori << 3) + perm;
            }   
            cube_corners[7] = ((cchk & 0xff8) % 24) | (cchk & 0x7);

            for (uint8_t i=0; i<11; i++) {
                uint8_t perm = utils_get_bits(data, 47 + i*4, 4);
                uint8_t ori = utils_get_bit(data, 91 + i);
                echk = echk ^ ((perm << 1) | ori );
                cube_edges[i] = (perm << 1) + ori;
            }   
            cube_edges[11] = echk;

            if ((memcmp(SOLVED_CORNERS, cube_corners, 8) == 0) &&
                (memcmp(SOLVED_EDGES, cube_edges, 12) == 0)) {

                #if DEBUG>0
                    Serial.println("[GAN] Solved!");
                #endif

                if (_gan_timer) {
                    _gan_timer = false;
                    float seconds = (millis() - _gan_start) / 1000.0;
                    #if DEBUG>0
                        Serial.printf("[GAN] Time: %7.3f seconds\n", seconds);
                    #endif
                }

            }

            gan_to_cube(cube_corners, cube_edges);

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
    _gan_characteristic_v2_read.disableNotify();
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

#include <Arduino.h>
#include <AES.h>
#include <bluefruit.h>

#include "config.h"
#include "bluetooth.h"
#include "utils.h"
#include "cube.h"
#include "ganv2.h"
#include "crypto.h"

static const uint8_t GANV2_CORNER_FACELET[8][3] = {
    {8, 9, 20}, // URF
    {6, 18, 38}, // UFL 
    {0, 36, 47}, // ULB
    {2, 45, 11}, // UBR
    {29, 26, 15}, // DFR
    {27, 44, 24}, // DLF
    {33, 53, 42}, // DBL
    {35, 17, 51}  // DRB
};
static const uint8_t GANV2_EDGE_FACELET[12][2] = {
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

extern uint8_t g_puzzle;

// ----------------------------------------------------------------------------
// AES
// ----------------------------------------------------------------------------

static const uint8_t GANV2_AES128_KEY[]  = {   1,   2,  66,  40,  49, 145,  22,   7,  32,   5,  24,  84,  66,  17,  18,  83 };
static const uint8_t GANV2_AES128_IV[]   = {  17,   3,  50,  40,  33,   1, 118,  39,  32, 149, 120,  20,  50,  18,   2,  67 };
static const uint8_t MOYUAI_AES128_KEY[] = {   5,  18,   2,  69,   2,   1,  41,  86,  18, 120,  18, 118, 129,   1,   8,   3 };
static const uint8_t MOYUAI_AES128_IV[]  = {   1,  68,  40,   6, 134,  33,  34,  40,  81,   5,   8,  49, 130,   2,  33,   6 };

void ganv2_init_aes128(uint8_t version) {
    
    // Key placeholder
    uint8_t key[16] = {0};
    uint8_t iv[16] = {0};

    // Copy seed keys
    if (0 == version) {
        memcpy(key, GANV2_AES128_KEY, 16);
        memcpy(iv, GANV2_AES128_IV, 16);
    } else {
        memcpy(key, MOYUAI_AES128_KEY, 16);
        memcpy(iv, MOYUAI_AES128_IV, 16);
    }

    // Get MAC
    unsigned char * mac = bluetooth_peer_addr();

    // Calculate keys
    for (uint8_t i=0; i<6; i++) {
        key[i] = ( key[i] + mac[5-i] ) % 255;
        iv[i] = ( iv[i] + mac[5-i] ) % 255;
    }

    // Init decoder
    aes128_init(key, iv);

}

// ----------------------------------------------------------------------------
// 
// ----------------------------------------------------------------------------

void ganv2_data_callback(uint8_t* data, uint16_t len) {

    static int16_t count_old = -1;
    
    if (aes128_decode(data, len)) {

        #if DEBUG > 2
            Serial.printf("[GANv2] Received: ");
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

            // Moves
            uint8_t turn = utils_get_bits(data, 12, 5);
            cube_move(turn >> 1, (turn & 0x01) ? 3 : 1);

        } else if (4 == mode) { // Cube state

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

            // Solved?
            cube_solved(cube_corners, cube_edges);

            // State
            cube_state(cube_corners, cube_edges, GANV2_CORNER_FACELET, GANV2_EDGE_FACELET);


        } else if (5 == mode) { // Hardware info
            
            char device[9] = {0};
            memcpy(device, &data[5], 8);
            bool gyro = (data[13] & 0x80) == 0x80;

            #if DEBUG > 0
                Serial.printf("[GANv2] Hardware info message received\n");
                Serial.printf("[GANv2] Device name     : %s\n", device);
                Serial.printf("[GANv2] Hardware version: %d.%d\n", data[1], data[2]);
                Serial.printf("[GANv2] Software version: %d.%d\n", data[3], data[4]);
                Serial.printf("[GANv2] Gyro enabled    : %s\n", gyro ? "yes": "no");
            #endif

        } else if (9 == mode) { // Battery

            #if DEBUG > 0
                Serial.printf("[GANv2] Battery         : %d%%\n", data[1]);
            #endif

            cube_set_battery(data[1]);

        }

    }

}

void ganv2_data_send(uint8_t* data, uint16_t len) {

    #if DEBUG > 1
        Serial.printf("[GANv2] Sending: 0x");
        for (uint16_t i=0; i<len; i++) {
            Serial.printf("%02X", data[i]);
        }
        Serial.println();
    #endif
    
    aes128_encode(data, len);
    ganv2_data_send_raw(data, len);

}

void ganv2_data_send(uint8_t opcode) {
    uint8_t data[20];
    data[0] = opcode;
    ganv2_data_send(data, sizeof(data));
}

void ganv2_reset() {
    uint8_t data[20] = {10, 5, 57, 119, 0, 0, 1, 35, 69, 103, 137, 171, 0, 0, 0, 0, 0, 0, 0, 0};
    ganv2_data_send(data, sizeof(data));
}

void ganv2_battery() {
    ganv2_data_send(GANV2_GET_BATTERY);    
}

// ----------------------------------------------------------------------------
// Bluetooth
// ----------------------------------------------------------------------------

const uint8_t GANV2_UUID_SERVICE_DATA[] = { 0x79, 0x41, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E };
const uint8_t GANV2_UUID_CHARACTERISTIC_READ[] = { 0xE4, 0xCC, 0xDB, 0xE2, 0x2A, 0x2A, 0x2F, 0xA3, 0xE9, 0x11, 0x67, 0xCD, 0xB6, 0x4C, 0xBE, 0x28 };
const uint8_t GANV2_UUID_CHARACTERISTIC_WRITE[] = { 0xE4, 0xCC, 0xDB, 0xE2, 0x2A, 0x2A, 0x2F, 0xA3, 0xE9, 0x11, 0x67, 0xCD, 0x4A, 0x4A, 0xBE, 0x28 };

BLEClientService _ganv2_service_data(GANV2_UUID_SERVICE_DATA);
BLEClientCharacteristic _ganv2_characteristic_read(GANV2_UUID_CHARACTERISTIC_READ);
BLEClientCharacteristic _ganv2_characteristic_write(GANV2_UUID_CHARACTERISTIC_WRITE);

void ganv2_data_send_raw(uint8_t* data, uint16_t len) {
    _ganv2_characteristic_write.write(data, len);
}

bool ganv2_start(uint16_t conn_handle) {

     // Discover GAN v2 data service (only one supported right now)
    if ( !_ganv2_service_data.discover(conn_handle) ) {
        #if DEBUG > 0
            Serial.println("[GANv2] GAN v2 data service not found. Skipping.");
        #endif
        return false;
    }
    #if DEBUG > 0
        Serial.println("[GANv2] GAN v2 data service found.");
    #endif

    // Discover GAN v2 write characteristic
    if ( ! _ganv2_characteristic_write.discover() ) {
        #if DEBUG > 0
            Serial.println("[GANv2] GAN v2 write characteristic not found. Skipping.");
        #endif
        return false;
    }
    #if DEBUG > 0
        Serial.println("[GANv2] GAN v2 write characteristic found.");
    #endif

    // Discover GAN v2 read characteristic
    if ( ! _ganv2_characteristic_read.discover() ) {
        #if DEBUG > 0
            Serial.println("[GANv2] GAN v2 read characteristic not found. Skipping.");
        #endif
        return false;
    }
    _ganv2_characteristic_read.enableNotify();
    #if DEBUG > 0
        Serial.println("[GANv2] GAN v2 read characteristic found. Subscribed.");
    #endif

    // Get cube type
    uint8_t version = (strncmp(bluetooth_peer_name(), "AiCube", 6) == 0) ? 1 : 0;
    #if DEBUG > 0
        if (0 == version) {
            Serial.println("[GANv2] Identified as GAN cube.");
        } else {
            Serial.println("[GANv2] Identified as MOYU AI cube.");
        }
    #endif

    // Set up encoder
    ganv2_init_aes128(version);

    // Query the cube
    ganv2_data_send(GANV2_GET_HARDWARE);
    ganv2_data_send(GANV2_GET_FACELETS);
    
    // Set cube as 3x3x3
    g_puzzle = PUZZLE_3x3x3;

    // Register callbacks
    cube_set_cube_callbacks(ganv2_battery, ganv2_reset);

    return true;
    
}

void ganv2_init() {

    // Init objects
    _ganv2_service_data.begin();
    _ganv2_characteristic_write.begin();
    _ganv2_characteristic_read.begin();
  
    // Notifications
    _ganv2_characteristic_read.setNotifyCallback([](BLEClientCharacteristic* chr, uint8_t* data, uint16_t len) {
        ganv2_data_callback(data, len);
    });

}
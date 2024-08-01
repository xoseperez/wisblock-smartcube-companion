#include <Arduino.h>
#include <AES.h>
#include <bluefruit.h>

#include "config.h"
#include "bluetooth.h"
#include "utils.h"
#include "cube.h"
#include "gan.h"
#include "crypto.h"

static const uint8_t GAN_CORNER_FACELET[8][3] = {
    {8, 9, 20}, // URF
    {6, 18, 38}, // UFL 
    {0, 36, 47}, // ULB
    {2, 45, 11}, // UBR
    {29, 26, 15}, // DFR
    {27, 44, 24}, // DLF
    {33, 53, 42}, // DBL
    {35, 17, 51}  // DRB
};
static const uint8_t GAN_EDGE_FACELET[12][2] = {
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

static const uint8_t GAN_AES128_KEY[]  = {   1,   2,  66,  40,  49, 145,  22,   7,  32,   5,  24,  84,  66,  17,  18,  83 };
static const uint8_t GAN_AES128_IV[]   = {  17,   3,  50,  40,  33,   1, 118,  39,  32, 149, 120,  20,  50,  18,   2,  67 };
static const uint8_t MOYUAI_AES128_KEY[] = {   5,  18,   2,  69,   2,   1,  41,  86,  18, 120,  18, 118, 129,   1,   8,   3 };
static const uint8_t MOYUAI_AES128_IV[]  = {   1,  68,  40,   6, 134,  33,  34,  40,  81,   5,   8,  49, 130,   2,  33,   6 };

void gan_init_aes128(uint8_t version) {
    
    // Key placeholder
    uint8_t key[16] = {0};
    uint8_t iv[16] = {0};

    // Copy seed keys
    if (0 == version) {
        memcpy(key, GAN_AES128_KEY, 16);
        memcpy(iv, GAN_AES128_IV, 16);
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

void gan_data_callback(uint8_t* data, uint16_t len) {

    #if DEBUG > 2
        Serial.printf("[GAN] Received: ");
        for (uint16_t i=0; i<len; i++) {
            Serial.printf("%02X", data[i]);
        }
        Serial.println();
    #endif
        
}

void gan_data_send(uint8_t* data, uint16_t len) {

    #if DEBUG > 1
        Serial.printf("[GAN] Sending: 0x");
        for (uint16_t i=0; i<len; i++) {
            Serial.printf("%02X", data[i]);
        }
        Serial.println();
    #endif
    
    aes128_encode(data, len);
    gan_data_send_raw(data, len);

}

void gan_data_send(uint8_t opcode) {
    uint8_t data[20];
    data[0] = opcode;
    gan_data_send(data, sizeof(data));
}

void gan_reset() {
}

void gan_battery() {
}

// ----------------------------------------------------------------------------
// Bluetooth
// ----------------------------------------------------------------------------

const uint8_t GAN_UUID_SERVICE_META[] =             { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x0A, 0x18, 0x00, 0x00 };
const uint8_t GAN_UUID_CHARACTERISTIC_VERSION[] =   { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x28, 0x2A, 0x00, 0x00 };
const uint8_t GAN_UUID_CHARACTERISTIC_HARDWARE[] =  { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x23, 0x2A, 0x00, 0x00 };

const uint8_t GAN_UUID_SERVICE_DATA[] =             { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xF0, 0xFF, 0x00, 0x00 };
const uint8_t GAN_UUID_CHARACTERISTIC_F2[] =        { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xF2, 0xFF, 0x00, 0x00 };
const uint8_t GAN_UUID_CHARACTERISTIC_F3[] =        { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xF3, 0xFF, 0x00, 0x00 };
const uint8_t GAN_UUID_CHARACTERISTIC_F5[] =        { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xF5, 0xFF, 0x00, 0x00 };
const uint8_t GAN_UUID_CHARACTERISTIC_F6[] =        { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xF6, 0xFF, 0x00, 0x00 };
const uint8_t GAN_UUID_CHARACTERISTIC_F7[] =        { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xF7, 0xFF, 0x00, 0x00 };

BLEClientService _gan_service_meta(GAN_UUID_SERVICE_META);
BLEClientCharacteristic _gan_characteristic_version(GAN_UUID_CHARACTERISTIC_VERSION);
BLEClientCharacteristic _gan_characteristic_hardware(GAN_UUID_CHARACTERISTIC_HARDWARE);

BLEClientService _gan_service_data(GAN_UUID_SERVICE_DATA);
BLEClientCharacteristic _gan_characteristic_f2(GAN_UUID_CHARACTERISTIC_F2);
BLEClientCharacteristic _gan_characteristic_f3(GAN_UUID_CHARACTERISTIC_F3);
BLEClientCharacteristic _gan_characteristic_f5(GAN_UUID_CHARACTERISTIC_F5);
BLEClientCharacteristic _gan_characteristic_f6(GAN_UUID_CHARACTERISTIC_F6);
BLEClientCharacteristic _gan_characteristic_f7(GAN_UUID_CHARACTERISTIC_F7);

void gan_data_send_raw(uint8_t* data, uint16_t len) {
    //_gan_characteristic_write.write(data, len);
}

bool gan_start(uint16_t conn_handle) {

    // Check if its a GAN-compatible cube
    if (!bluetooth_discover_service(_gan_service_meta, "GAN", "meta")) return false;
    if (!bluetooth_discover_characteristic(_gan_characteristic_version, "GAN", "version")) return false;
    if (!bluetooth_discover_characteristic(_gan_characteristic_hardware, "GAN", "hardware")) return false;

    if (!bluetooth_discover_service(_gan_service_data, "GAN", "data")) return false;
    if (!bluetooth_discover_characteristic(_gan_characteristic_f2, "GAN", "f2")) return false;
    if (!bluetooth_discover_characteristic(_gan_characteristic_f3, "GAN", "f3")) return false;
    if (!bluetooth_discover_characteristic(_gan_characteristic_f5, "GAN", "f5")) return false;
    if (!bluetooth_discover_characteristic(_gan_characteristic_f6, "GAN", "f6")) return false;
    if (!bluetooth_discover_characteristic(_gan_characteristic_f7, "GAN", "f7")) return false;

    // Enable notifications
    _gan_characteristic_version.enableNotify();

    // Set up encoder
    //gan_init_aes128(version);

    // Set cube as 3x3x3
    g_puzzle = PUZZLE_3x3x3;

    // Register callbacks
    cube_set_cube_callbacks(gan_battery, gan_reset);

    return true;
    
}

void gan_init() {

    // Init objects
    _gan_service_meta.begin();
    _gan_characteristic_version.begin();
    _gan_characteristic_hardware.begin();
    _gan_service_data.begin();
    _gan_characteristic_f2.begin();
    _gan_characteristic_f3.begin();
    _gan_characteristic_f5.begin();
    _gan_characteristic_f6.begin();
    _gan_characteristic_f7.begin();

    // Notifications
    _gan_characteristic_version.setNotifyCallback([](BLEClientCharacteristic* chr, uint8_t* data, uint16_t len) {
        gan_data_callback(data, len);
        _gan_characteristic_version.disableNotify();
    });

}
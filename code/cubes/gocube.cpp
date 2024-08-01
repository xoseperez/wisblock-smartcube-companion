#include <Arduino.h>
#include <bluefruit.h>

#include "config.h"
#include "bluetooth.h"
#include "utils.h"
#include "cube.h"
#include "gocube.h"

extern uint8_t g_puzzle;

static const unsigned char GOCUBE_AXIS_PERM[] = {5, 2, 0, 3, 1, 4};
static const unsigned char GOCUBE_FACE_PERM[] = {0, 1, 2, 5, 8, 7, 6, 3};
static const unsigned char GOCUBE_FACE_OFFSET[] = {0, 0, 6, 2, 0, 0};
static const char GOCUBE_FACES[] = "BFUDRL";

// ----------------------------------------------------------------------------
// 
// ----------------------------------------------------------------------------

void gocube_data_callback(uint8_t* data, uint16_t len) {

    #if DEBUG > 1
        Serial.printf("[GOC] Received: ");
        for (uint16_t i=0; i<len; i++) {
            Serial.printf("%02X", data[i]);
        }
        Serial.println();
    #endif

    //if ((data[0] != 0x2A) || (data[len-2] != 0x0D) || (data[len-1] != 0x0A)) {
    if (data[0] != 0x2A) {
        Serial.println("[GOC] Wrond message format");
        return;
    }
    
    uint8_t mode = data[2];
    len -= 6;
    
    // Cube move
    if (1 == mode) {
        
        // Decode moves
        for (uint8_t i=0; i<len; i+=2) {
            uint8_t face = GOCUBE_AXIS_PERM[data[3 + i] >> 1];
            uint8_t count = (data[3 + i] & 0x01) ? 3 : 1;
            cube_move(face, count);
        }
        
        // Update state
        gocube_data_send(GOCUBE_GET_STATE);

    // Cube state
    } else if (2 == mode) { 

        char facelet[55] = {0};

        for (uint8_t a=0; a<6; a++) {
            unsigned char axis = GOCUBE_AXIS_PERM[a] * 9;
            unsigned char offset = GOCUBE_FACE_OFFSET[a];
            facelet[axis + 4] = GOCUBE_FACES[data[3+a*9]];
            for (uint8_t i=0; i<8; i++) {
                facelet[axis + GOCUBE_FACE_PERM[(i+offset) % 8]] = GOCUBE_FACES[data[3+a*9+i+1]];
            }
        }

        // Solved
        cube_solved(facelet);

        // State
        cube_state(facelet, 54);


    // Battery
    } else if (5 == mode) {
        
        #if DEBUG > 0
            Serial.printf("[GOC] Battery         : %d%%\n", data[3]);
        #endif

        cube_set_battery(data[3]);

    // Cube type
    } else if (8 == mode) {

    }

}

void gocube_data_send(uint8_t* data, uint16_t len) {

    #if DEBUG > 1
        Serial.printf("[GOC] Sending: 0x");
        for (uint16_t i=0; i<len; i++) {
            Serial.printf("%02X", data[i]);
        }
        Serial.println();
    #endif
    
    gocube_data_send_raw(data, len);

}

void gocube_data_send(uint8_t opcode) {
    uint8_t data[1];
    data[0] = opcode;
    gocube_data_send(data, sizeof(data));
}

void gocube_reset() {
}

void gocube_battery() {
    gocube_data_send(GOCUBE_GET_BATTERY);    
}

// ----------------------------------------------------------------------------
// Bluetooth
// ----------------------------------------------------------------------------

const uint8_t GOCUBE_UUID_SERVICE_DATA[] = { 0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E };
const uint8_t GOCUBE_UUID_CHARACTERISTIC_READ[] = { 0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x03, 0x00, 0x40, 0x6E };
const uint8_t GOCUBE_UUID_CHARACTERISTIC_WRITE[] = { 0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x02, 0x00, 0x40, 0x6E };

BLEClientService _gocube_service_data(GOCUBE_UUID_SERVICE_DATA);
BLEClientCharacteristic _gocube_characteristic_read(GOCUBE_UUID_CHARACTERISTIC_READ);
BLEClientCharacteristic _gocube_characteristic_write(GOCUBE_UUID_CHARACTERISTIC_WRITE);

void gocube_data_send_raw(uint8_t* data, uint16_t len) {
    _gocube_characteristic_write.write(data, len);
}

bool gocube_start(uint16_t conn_handle) {

    // Discover services & characteristics
    if (!bluetooth_discover_service(&_gocube_service_data, "GOC", "data")) return false;
    if (!bluetooth_discover_characteristic(&_gocube_characteristic_write, "GOC", "write")) return false;
    if (!bluetooth_discover_characteristic(&_gocube_characteristic_read, "GOC", "read")) return false;

    // Enable notifications
    _gocube_characteristic_read.enableNotify();

    // Query the cube
    gocube_data_send(GOCUBE_GET_STATE);
    
    // Set cube as 3x3x3
    g_puzzle = PUZZLE_3x3x3;

    // Register callbacks
    cube_set_cube_callbacks(gocube_battery, gocube_reset);

    return true;
    
}

void gocube_init() {

    // Notifications
    _gocube_characteristic_read.setNotifyCallback([](BLEClientCharacteristic* chr, uint8_t* data, uint16_t len) {
        gocube_data_callback(data, len);
    }, true);

    // Init objects
    _gocube_service_data.begin();
    _gocube_characteristic_write.begin();
    _gocube_characteristic_read.begin();
  
}
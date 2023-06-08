#include <Arduino.h>
#include <bluefruit.h>

#include "config.h"
#include "bluetooth.h"
#include "cube.h"
#include "giiker.h"

static const uint8_t GIIKER_FACE_MAP[] = {5, 3, 4, 0, 1, 2};
static const unsigned char GIIKER_CORNER_FACELET_2x2x2[8][3] = {
	{11, 6, 13},
	{9, 3, 4},
	{8, 17, 2},
	{10, 12, 19},
	{22, 15, 7},
	{20, 5, 1},
	{21, 0, 16},
	{23, 18, 14},
};
static const unsigned char GIIKER_CORNER_FACELET_3x3x3[8][3] = {
	{26, 15, 29},
	{20, 8, 9},
	{18, 38, 6},
	{24, 27, 44},
	{51, 35, 17},
	{45, 11, 2},
	{47, 0, 36},
	{53, 42, 33}
};
static const unsigned char GIIKER_EDGE_FACELET[12][2] = {
	{25, 28},
	{23, 12},
	{19, 7},
	{21, 41},
	{32, 16},
	{5, 10},
	{3, 37},
	{30, 43},
	{52, 34},
	{48, 14},
	{46, 1},
	{50, 39}
};
static const uint8_t GIIKER_KEY[] = { 176, 81, 104, 224, 86, 137, 237, 119, 38, 26, 193, 161, 210, 126, 150, 81, 93, 13, 236, 249, 89, 235, 88, 24, 113, 81, 214, 131, 130, 199, 2, 169, 39, 165, 171, 41 };

extern uint8_t g_puzzle;

// ----------------------------------------------------------------------------
// 
// ----------------------------------------------------------------------------

bool giiker_decode(uint8_t* data, uint8_t* output, uint8_t len) {

    // Right length?
    if (len != 20) return false;

    // Encrypted?
    if (data[18] == 0xA7) {

        uint8_t k1 = (data[19] >> 4) & 0x0F;
        uint8_t k2 = (data[19] >> 0) & 0x0F;
        for (uint8_t i=0; i<18; i++) {
            data[i] = data[i] + GIIKER_KEY[i+k1] + GIIKER_KEY[i+k2];
        }
    }

    for (uint8_t i=0; i<18; i++) {
        output[i*2+0] = (data[i] >> 4) & 0x0F;
        output[i*2+1] = (data[i] >> 0) & 0x0F;
    }

    return true;

}

void giiker_data_callback(uint8_t* data, uint16_t len) {

    uint8_t decoded[36] = {0};
    if (giiker_decode(data, decoded, len)) {

        #if DEBUG > 1
            Serial.printf("[GII] DATA Received: ");
            for (uint16_t i=0; i<36; i++) {
                Serial.printf("%01X", decoded[i]);
            }
            Serial.println();
        #endif
        
        uint8_t cube_corners[8];
        uint8_t cube_edges_orientation[12];
        uint8_t cube_edges[12];
        int8_t mask[] = { -1, 1, -1, 1, 1, -1, 1, -1 };

        // Corners
        for (uint8_t i=0; i<8; i++) {
            cube_corners[i] = (decoded[i] - 1) | ((3 + decoded[i + 8] * mask[i]) % 3) << 3;
        }   
        
        // Edges (only for 3x3x3)
        if (g_puzzle == PUZZLE_3x3x3) {
            uint8_t k=0;
            for (uint8_t i=0; i<3; i++) {
                for (uint8_t j=8; j!=0; j>>=1) {
                    if (decoded[i + 28] & j) {
                        cube_edges_orientation[k++] = 1;
                    } else {
                        cube_edges_orientation[k++] = 0;
                    }
                }
            }
            for (uint8_t i=0; i<12; i++) {
                cube_edges[i] = (decoded[i + 16] - 1) << 1 | cube_edges_orientation[i];
            }   
        }

        // Moves
        cube_move(GIIKER_FACE_MAP[decoded[32]-1], decoded[33]);

        // Solved
        cube_solved(cube_corners, cube_edges);

        // State
        if (g_puzzle == PUZZLE_3x3x3) {
            cube_state(cube_corners, cube_edges, GIIKER_CORNER_FACELET_3x3x3, GIIKER_EDGE_FACELET);
        } else {
            cube_state(cube_corners, cube_edges, GIIKER_CORNER_FACELET_2x2x2, GIIKER_EDGE_FACELET);
        }

        
    }

}

void giiker_rw_callback(uint8_t* data, uint16_t len) {

    #if DEBUG > 1
        Serial.printf("[GII] READ Received: ");
        for (uint16_t i=0; i<len; i++) {
            Serial.printf("%02X", data[i]);
        }
        Serial.println();
    #endif
        
    if (data[0] == GIIKER_GET_BATTERY) { // B5
        cube_set_battery(data[1]);
        #if DEBUG > 0
            Serial.printf("[GII] Battery         : %d%%\n", data[1]);
        #endif
    }

    if (data[0] == GIIKER_GET_FIRMWARE) { // B7
        #if DEBUG > 0
            Serial.printf("[GII] Device name     : %s\n", bluetooth_peer_name());
            Serial.printf("[GII] Software version: 0x%02X\n", data[1]);
        #endif
    }

    if (data[0] == GIIKER_GET_MOVES) { // CC
        #if DEBUG > 0
            uint32_t moves = (data[1] << 24) + (data[2] << 16) + (data[3] << 8) + data[4];
            Serial.printf("[GII] Number of moves : %d\n", moves);
        #endif
    }

}

void giiker_data_send(uint8_t* data, uint16_t len) {

    #if DEBUG > 1
        Serial.printf("[GII] Sending: 0x");
        for (uint16_t i=0; i<len; i++) {
            Serial.printf("%02X", data[i]);
        }
        Serial.println();
    #endif
    
    giiker_data_send_raw(data, len);

}

void giiker_data_send(uint8_t opcode) {
    uint8_t data[] = { opcode };
    giiker_data_send(data, sizeof(data));
}

void giiker_battery() {
    giiker_data_send(GIIKER_GET_BATTERY);    
}

void giiker_reset() {
    giiker_data_send(GIIKER_RESET);    
}

// ----------------------------------------------------------------------------
// Bluetooth
// ----------------------------------------------------------------------------

const uint8_t GIIKER_UUID_SERVICE_DATA[] = { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xDB, 0xAA, 0x00, 0x00 };
const uint8_t GIIKER_UUID_CHARACTERISTIC_DATA[] = { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xDC, 0xAA, 0x00, 0x00 };

const uint8_t GIIKER_UUID_SERVICE_RW[] = { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xAA, 0xAA, 0x00, 0x00 };
const uint8_t GIIKER_UUID_CHARACTERISTIC_WRITE[] = { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xAC, 0xAA, 0x00, 0x00 };
const uint8_t GIIKER_UUID_CHARACTERISTIC_READ[] = { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xAB, 0xAA, 0x00, 0x00 };

BLEClientService _giiker_service_data(GIIKER_UUID_SERVICE_DATA);
BLEClientCharacteristic _giiker_characteristic_data(GIIKER_UUID_CHARACTERISTIC_DATA);

BLEClientService _giiker_service_rw(GIIKER_UUID_SERVICE_RW);
BLEClientCharacteristic _giiker_characteristic_write(GIIKER_UUID_CHARACTERISTIC_WRITE);
BLEClientCharacteristic _giiker_characteristic_read(GIIKER_UUID_CHARACTERISTIC_READ);

void giiker_data_send_raw(uint8_t* data, uint16_t len) {
    _giiker_characteristic_write.write(data, len);
}

bool giiker_start(uint16_t conn_handle) {

     // Discover Giiker data service (only one supported right now)
    if ( !_giiker_service_data.discover(conn_handle) ) {
        #if DEBUG > 0
            Serial.println("[GII] GIIKER data service not found. Skipping.");
        #endif
        return false;
    }
    #if DEBUG > 0
        Serial.println("[GII] GIIKER data service found.");
    #endif

    // Discover Giiker data characteristic
    if ( ! _giiker_characteristic_data.discover() ) {
        #if DEBUG > 0
            Serial.println("[GII] GIIKER data characteristic not found. Skipping.");
        #endif
        return false;
    }
    _giiker_characteristic_data.enableNotify();
    #if DEBUG > 0
        Serial.println("[GII] GIIKER data characteristic found. Subscribed.");
    #endif

    // For the battery services, even if we don't find them we go on

    // Discover Giiker rw service (only one supported right now)
    if ( !_giiker_service_rw.discover(conn_handle) ) {
        #if DEBUG > 0
            Serial.println("[GII] GIIKER rw service not found. Skipping.");
        #endif
        return true;
    }
    #if DEBUG > 0
        Serial.println("[GII] GIIKER rw service found.");
    #endif

    // Discover Giiker write characteristic
    if ( ! _giiker_characteristic_write.discover() ) {
        #if DEBUG > 0
            Serial.println("[GII] GIIKER write characteristic not found.");
        #endif
        return true;
    }
    #if DEBUG > 0
        Serial.println("[GII] GIIKER write characteristic found.");
    #endif

    // Discover Giiker read characteristic
    if ( ! _giiker_characteristic_read.discover() ) {
        #if DEBUG > 0
            Serial.println("[GII] GIIKER read characteristic not found.");
        #endif
        return true;
    }
    _giiker_characteristic_read.enableNotify();
    #if DEBUG > 0
        Serial.println("[GII] GIIKER read characteristic found. Subscribed.");
    #endif

    // Get info
    giiker_data_send(GIIKER_GET_FIRMWARE);
    giiker_data_send(GIIKER_GET_MOVES);

    // Define puzzle
    if (strncmp("Gi2", bluetooth_peer_name(), 3) == 0) {
        g_puzzle = PUZZLE_2x2x2;
    } else {
        g_puzzle = PUZZLE_3x3x3;
    }
    
    // Register callbacks
    cube_set_cube_callbacks(giiker_battery, giiker_reset);

    return true;
    
}

void giiker_init() {

    // Init objects
    _giiker_service_data.begin();
    _giiker_characteristic_data.begin();
    _giiker_service_rw.begin();
    _giiker_characteristic_read.begin();
    _giiker_characteristic_write.begin();
    
    // Notifications
    _giiker_characteristic_data.setNotifyCallback([](BLEClientCharacteristic* chr, uint8_t* data, uint16_t len) {
        giiker_data_callback(data, len);
    });
    _giiker_characteristic_read.setNotifyCallback([](BLEClientCharacteristic* chr, uint8_t* data, uint16_t len) {
        giiker_rw_callback(data, len);
        _giiker_characteristic_read.disableNotify();
    });

}
#include <Arduino.h>
#include <AES.h>
#include <bluefruit.h>

#include "bluetooth.h"
#include "config.h"
#include "utils.h"
#include "giiker.h"
#include "display.h"

uint32_t _giiker_start = 0;
bool _giiker_timer = 0;

static const char GIIKER_FACES[] = "URFDLB";
static const char GIIKER_MOVES[] = "BDLURF";
static const uint8_t GIIKER_SOLVED_CORNERS[] = {0, 1, 2, 3, 4, 5, 6, 7};
static const uint8_t GIIKER_SOLVED_EDGES[] = {0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22};
static const uint8_t GIIKER_CORNER_FACELET[8][3] = {
	{26, 15, 29},
	{20, 8, 9},
	{18, 38, 6},
	{24, 27, 44},
	{51, 35, 17},
	{45, 11, 2},
	{47, 0, 36},
	{53, 42, 3}
};
static const uint8_t GIIKER_EDGE_FACELET[12][2] = {
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
	{50, 3}
};
static const uint8_t GIIKER_KEY[] = { 176, 81, 104, 224, 86, 137, 237, 119, 38, 26, 193, 161, 210, 126, 150, 81, 93, 13, 236, 249, 89, 235, 88, 24, 113, 81, 214, 131, 130, 199, 2, 169, 39, 165, 171, 41 };


// ----------------------------------------------------------------------------
// 
// ----------------------------------------------------------------------------

void giiker_to_cube(uint8_t * corners, uint8_t * edges) {

    #if DEBUG>1
        Serial.printf("[GII] Corners: ");
        for (uint16_t i=0; i<8; i++) {
            Serial.printf("%d ", corners[i]);
        }
        Serial.println();
        Serial.printf("[GII] Edges: ");
        for (uint16_t i=0; i<12; i++) {
            Serial.printf("%d ", edges[i]);
        }
        Serial.println();
    #endif

    char facelets[55] = {0};
    for (uint8_t i=0; i<54; i++) {
        facelets[i] = GIIKER_FACES[int(i / 9)];
    }
    for (uint8_t c=0; c<8; c++) {
        uint8_t j = corners[c] & 0x07;
        uint8_t ori = corners[c] >> 3;
        for (uint8_t n=0; n<3; n++) {
            facelets[GIIKER_CORNER_FACELET[c][(n + ori) % 3]] = GIIKER_FACES[int(GIIKER_CORNER_FACELET[j][n] / 9)];
        }
    }
    for (uint8_t e=0; e<12; e++) {
        uint8_t j = edges[e] >> 1;
        uint8_t ori = edges[e] & 0x01;
        for (uint8_t n=0; n<2; n++) {
            facelets[GIIKER_EDGE_FACELET[e][(n + ori) % 2]] = GIIKER_FACES[int(GIIKER_EDGE_FACELET[j][n] / 9)];
        }
    }

    #if DEBUG>1
        Serial.printf("[GII] State: %s\n", facelets);
    #endif

    display_update_cube(facelets);

}

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
            Serial.printf("[GII] Received: ");
            for (uint16_t i=0; i<36; i++) {
                Serial.printf("%01X", decoded[i]);
            }
            Serial.println();
        #endif
        
        uint8_t cube_corners[8];
        uint8_t cube_edges_orientation[12];
        uint8_t cube_edges[12];
        int8_t mask[] = { -1, 1, -1, 1, 1, -1, 1, -1 };

        for (uint8_t i=0; i<8; i++) {
            cube_corners[i] = (decoded[i] - 1) | ((3 + decoded[i + 8] * mask[i]) % 3) << 3;
        }   
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

        if ((memcmp(GIIKER_SOLVED_CORNERS, cube_corners, 8) == 0) &&
            (memcmp(GIIKER_SOLVED_EDGES, cube_edges, 12) == 0)) {

            #if DEBUG>0
                Serial.println("[GII] Solved!");
            #endif

            if (_giiker_timer) {
                _giiker_timer = false;
                float seconds = (millis() - _giiker_start) / 1000.0;
                #if DEBUG>0
                    Serial.printf("[GII] Time: %7.3f seconds\n", seconds);
                #endif
            }

        }

        giiker_to_cube(cube_corners, cube_edges);

        // Moves
        static uint8_t uturns = 0;
        if (!_giiker_timer) {
            if ((decoded[32] == 4) && (decoded[33] == 1)) {
                uturns+=1;
            } else {
                uturns=0;
            }
            if (uturns == 4) {
                uturns=0;
                #if DEBUG > 0
                    Serial.println("[GII] 4 U turns in a row! Starting timer.");
                #endif
                _giiker_timer = true;
                _giiker_start = millis();
            }
        }

        #if DEBUG > 1
            Serial.print("[GII] Movement: ");
            Serial.print(GIIKER_MOVES[decoded[32]-1]);
            if (decoded[33] == 2) Serial.print("2");
            if (decoded[33] == 3) Serial.print("'");
            Serial.println();
        #endif
        
    }

}

// ----------------------------------------------------------------------------
// Bluetooth
// ----------------------------------------------------------------------------

const uint8_t GIIKER_UUID_SERVICE_DATA[] = { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xDB, 0xAA, 0x00, 0x00 };
const uint8_t GIIKER_UUID_CHARACTERISTIC_READ[] = { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xDC, 0xAA, 0x00, 0x00 };

BLEClientService _giiker_service_data(GIIKER_UUID_SERVICE_DATA);
BLEClientCharacteristic _giiker_characteristic_read(GIIKER_UUID_CHARACTERISTIC_READ);

void giiker_stop() {
    _giiker_characteristic_read.disableNotify();
}

bool giiker_start(uint16_t conn_handle) {

     // Discover GAN v2 data service (only one supported right now)
   _giiker_service_data.begin();
    if ( !_giiker_service_data.discover(conn_handle) ) {
        #if DEBUG > 0
            Serial.println("[GII] GIIKER data service not found. Disconnecting.");
        #endif
        return false;
    }
    #if DEBUG > 0
        Serial.println("[GII] GIIKER data service found.");
    #endif

    // Discover GAN v2 read characteristic
    _giiker_characteristic_read.begin();
    if ( ! _giiker_characteristic_read.discover() ) {
        #if DEBUG > 0
            Serial.println("[GII] GIIKER read characteristic not found. Disconnecting.");
        #endif
        return false;
    }
    _giiker_characteristic_read.setNotifyCallback([](BLEClientCharacteristic* chr, uint8_t* data, uint16_t len) {
        giiker_data_callback(data, len);
    });
    _giiker_characteristic_read.enableNotify();
    #if DEBUG > 0
        Serial.println("[GII] GIIKER read characteristic found. Subscribed.");
    #endif

    // Clear display
    display_clear();

    return true;
    
}

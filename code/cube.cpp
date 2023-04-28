#include <Arduino.h>

#include "config.h"
#include "cube.h"

uint32_t _cube_start = 0;
uint32_t _cube_time = 0;
uint8_t _cube_status = 0; // 0 -> idle, 1 -> solved, 2-> ready, 3-> counting
uint8_t _cube_cubelets[55] = {0};
bool _cube_updated = false;
uint8_t _cube_uturns = 0;

static const char CUBE_FACES[] = "URFDLB";
static const uint8_t CUBE_SOLVED_CORNERS[] = {0, 1, 2, 3, 4, 5, 6, 7};
static const uint8_t CUBE_SOLVED_EDGES[] = {0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22};

void cube_reset() {
    _cube_status = 0;
    _cube_updated = false;
    _cube_uturns = 0;
}

unsigned char cube_status() {
    return _cube_status;
}

bool cube_updated() {
    bool ret = _cube_updated;
    _cube_updated = false;
    return ret;
}

unsigned long cube_time() {
    if (_cube_status == 3) {
        return millis() - _cube_start;
    }
    return _cube_time;
}

uint8_t * cube_cubelets() {
    return _cube_cubelets;
}

bool cube_solved(uint8_t * corners, uint8_t * edges) {

    bool solved = 
        ((memcmp(CUBE_SOLVED_CORNERS, corners, 8) == 0) &&
        (memcmp(CUBE_SOLVED_EDGES, edges, 12) == 0));

    if ((solved) && (_cube_status == 3)) {
        _cube_time = cube_time();
        _cube_status = 1;
        #if DEBUG>0
            Serial.printf("[CUB] Solved in %7.3f seconds\n", cube_time() / 1000.0);
        #endif
    }

    return solved;

}

void cube_move(uint8_t face, uint8_t dir) {

    // Moves
    if (_cube_status == 2) {
        _cube_status = 3;
        _cube_start = millis();
        #if DEBUG > 0
            Serial.println("[CUB] Starting timer.");
        #endif
    }

    if (_cube_status < 2) {
        if ((0 == face) && (0 == dir)) {
            _cube_uturns += 1;
        } else {
            _cube_uturns = 0;
        }
        if (_cube_uturns == 4) {
            _cube_uturns = 0;
            _cube_status = 2;
            #if DEBUG > 0
                Serial.println("[CUB] 4 U turns in a row! Starting timer on next move.");
            #endif
        }
    }

    #if DEBUG > 1
        Serial.print("[CUB] Move: ");
        Serial.print(CUBE_FACES[face]);
        if (1 == dir) Serial.print("'");
        if (2 == dir) Serial.print("2");
        Serial.println();
    #endif
    
}

void cube_state(uint8_t * corners, uint8_t * edges, const unsigned char cfacelet[8][3], const unsigned char efacelet[12][2]) {

    #if DEBUG>1
        Serial.printf("[CUB] Corners: ");
        for (uint16_t i=0; i<8; i++) {
            Serial.printf("%d ", corners[i]);
        }
        Serial.println();
        Serial.printf("[CUB] Edges: ");
        for (uint16_t i=0; i<12; i++) {
            Serial.printf("%d ", edges[i]);
        }
        Serial.println();
    #endif

    for (uint8_t i=0; i<54; i++) {
        _cube_cubelets[i] = CUBE_FACES[int(i / 9)];
    }
    for (uint8_t c=0; c<8; c++) {
        uint8_t j = corners[c] & 0x07;
        uint8_t ori = corners[c] >> 3;
        for (uint8_t n=0; n<3; n++) {
            _cube_cubelets[cfacelet[c][(n + ori) % 3]] = CUBE_FACES[int(cfacelet[j][n] / 9)];
        }
    }
    for (uint8_t e=0; e<12; e++) {
        uint8_t j = edges[e] >> 1;
        uint8_t ori = edges[e] & 0x01;
        for (uint8_t n=0; n<2; n++) {
            _cube_cubelets[efacelet[e][(n + ori) % 2]] = CUBE_FACES[int(efacelet[j][n] / 9)];
        }
    }

    #if DEBUG>1
        Serial.printf("[CUB] State: %s\n", _cube_cubelets);
    #endif

    _cube_updated = true;

}


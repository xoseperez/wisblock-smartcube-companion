#include <Arduino.h>

#include "config.h"
#include "cube.h"
#include "cubes/ganv2.h"
#include "cubes/giiker.h"

// state
uint8_t _cube_cubelets[55] = {0};
bool _cube_updated = false;
uint8_t _cube_battery = 0xFF;
bool _cube_connected = false;

// metrics
bool _cube_running_metrics = false;
uint32_t _cube_last_move_millis = 0;
uint32_t _cube_start = 0;
uint32_t _cube_time = 0;
uint8_t _cube_turns = 0;

void (*_cube_callback)(uint8_t event);

static const char CUBE_FACES[] = "URFDLB";
static const uint8_t CUBE_SOLVED_CORNERS[] = {0, 1, 2, 3, 4, 5, 6, 7};
static const uint8_t CUBE_SOLVED_EDGES[] = {0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22};

// ----------------------------------------------------------------------------
// Public
// ----------------------------------------------------------------------------

void cube_set_callback(void (*callback)(uint8_t event)) {
    _cube_callback = callback;
}

void cube_reset() {
    _cube_turns = 0;
    _cube_updated = false;
}

uint8_t cube_get_battery() {
    return _cube_battery;
}

bool cube_updated() {
    bool ret = _cube_updated;
    _cube_updated = false;
    return ret;
}

uint8_t * cube_cubelets() {
    return _cube_cubelets;
}

bool cube_bind(uint8_t conn_handle) {

    // Walk through cubes to identofy the connection
    _cube_connected = false;
    _cube_connected = _cube_connected || ganv2_start(conn_handle);
    _cube_connected = _cube_connected || giiker_start(conn_handle);

    if (_cube_connected && _cube_callback) _cube_callback(CUBE_EVENT_CONNECTED);
    return _cube_connected;

}

void cube_unbind() {
    //cube_reset();
    _cube_battery = 0xFF;
    if (_cube_connected && _cube_callback) _cube_callback(CUBE_EVENT_DISCONNECTED);
    _cube_connected = false;
}

void cube_setup() {
    ganv2_init();
    giiker_init();
}

// ----------------------------------------------------------------------------
// Metrics
// ----------------------------------------------------------------------------

void cube_metrics_start() {
    _cube_start = _cube_last_move_millis;
    _cube_turns = 0;
    _cube_running_metrics = true;
}

void cube_metrics_end() {
    _cube_time = _cube_last_move_millis - _cube_start;
    _cube_running_metrics = false;
}

unsigned long cube_time() {
    if (_cube_running_metrics) {
        return millis() - _cube_start;
    }
    return _cube_time;
}

unsigned short cube_turns() {
    return _cube_turns;
}

// ----------------------------------------------------------------------------
// Protected
// Methods called only by cubes
// ----------------------------------------------------------------------------

void cube_set_battery(uint8_t battery) {
    _cube_battery = battery;
}

bool cube_solved(uint8_t * corners, uint8_t * edges) {

    bool solved = 
        ((memcmp(CUBE_SOLVED_CORNERS, corners, 8) == 0) &&
        (memcmp(CUBE_SOLVED_EDGES, edges, 12) == 0));

    if (solved) {
        if (_cube_callback) _cube_callback(CUBE_EVENT_SOLVED);
    }

    return solved;

}

void cube_move(uint8_t face, uint8_t dir) {

    _cube_last_move_millis = millis();

    if (_cube_callback) _cube_callback(CUBE_EVENT_MOVE);
    
    // Metrics
    if (_cube_running_metrics) _cube_turns++;

    // Check U turns
    static unsigned long uturns = 0;
    if ((0 == face) && (0 == dir)) {
        uturns += 1;
    } else {
        uturns = 0;
    }
    if (uturns == 4) {
        uturns = 0;
        uturns = 2;
        if (_cube_callback) _cube_callback(CUBE_EVENT_4UTURNS);
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


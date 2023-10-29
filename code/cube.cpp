#include <Arduino.h>

#include "config.h"
#include "cube.h"
#include "ring.h"
#include "cubes/ganv2.h"
#include "cubes/giiker.h"
#include "cubes/gocube.h"

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
uint16_t _cube_turns = 0;

char _cube_step[4] = {0};

void (*_cube_callback)(uint8_t event, uint8_t * data);
void (*_cube_reset_callback)();
void (*_cube_battery_callback)();

static const char CUBE_FACES[] = "URFDLB";
static const uint8_t CUBE_SOLVED_CORNERS[] = {0, 1, 2, 3, 4, 5, 6, 7};
static const uint8_t CUBE_SOLVED_EDGES[] = {0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22};
static const char * CUBE_SOLVED_FACELET_3x3x3 = "UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB";
static const unsigned char CUBE_SCRAMBLE_SIZES[] = { 20, 9, 0, 0, 0, 0};

extern uint8_t g_puzzle;

// ----------------------------------------------------------------------------
// Public
// ----------------------------------------------------------------------------

void cube_set_callback(void (*callback)(uint8_t event, uint8_t * data)) {
    _cube_callback = callback;
}

void cube_reset() {
    if (_cube_reset_callback) _cube_reset_callback();
}

uint8_t cube_get_battery() {
    // if (_cube_battery_callback) _cube_battery_callback(); // TODO: wait for battery update?
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

    // Walk through cubes to identify the connection
    _cube_connected = false;
    _cube_connected = _cube_connected || ganv2_start(conn_handle);
    _cube_connected = _cube_connected || giiker_start(conn_handle);
    _cube_connected = _cube_connected || gocube_start(conn_handle);

    if (_cube_connected && _cube_callback) _cube_callback(CUBE_EVENT_CONNECTED, nullptr);
    return _cube_connected;

}

void cube_unbind() {
    _cube_battery = 0xFF;
    if (_cube_connected && _cube_callback) _cube_callback(CUBE_EVENT_DISCONNECTED, nullptr);
    _cube_connected = false;
    _cube_reset_callback = nullptr;
    _cube_battery_callback = nullptr;
}

void cube_setup() {
    ganv2_init();
    giiker_init();
    gocube_init();
    randomSeed(analogRead(WB_A1));
}

// ----------------------------------------------------------------------------
// Scramble
// ----------------------------------------------------------------------------

void cube_scramble(uint8_t puzzle, Ring * moves) {

    uint8_t last_face = 0xFF;
    uint8_t last_group = 0xFF;
    uint8_t last_group_count = 0;

    // clear buffer
    moves->clear();

    uint8_t size = CUBE_SCRAMBLE_SIZES[puzzle];
    if (size == 0) return;
    if (size > moves->size()) return;

    #if DEBUG>0
        Serial.print("[CUB] Scramble: ");
    #endif

    uint8_t max_group = (puzzle == PUZZLE_2x2x2) ? 2 : 3;

    for (uint8_t i=0; i<size; i++) {

        uint8_t face;

        do {
            
            do {

                face = random(0, 6);

                // For Gi2 we only want URF
                if (g_puzzle == PUZZLE_2x2x2) {
                    if (face > 2) face -= 3;
                }

            } while (face == last_face);
            
            if (last_group == face % 3) {
                last_group_count++;
            } else {
                last_group = face % 3;
                last_group_count = 1;
            }
        
        } while (last_group_count>=max_group);
        last_face = face;

        uint8_t count = random(1, 4);
        uint8_t move = (count << 4) + (face & 0x0F);
        #if DEBUG>0
            Serial.print(cube_turn_text(move));
            Serial.print(" ");
        #endif
        moves->append(move);

    }
    
    #if DEBUG>0
        Serial.println();
    #endif
}

char * cube_turn_text(uint8_t code, bool space) {

    uint8_t pos = 0;
    uint8_t face = code & 0x0F;
    uint8_t count = (code & 0xF0) >> 4;

    _cube_step[pos++] = CUBE_FACES[face];
    if (2 == count) _cube_step[pos++] = '2';
    if (3 == count) _cube_step[pos++] = '\'';
    if (space) _cube_step[pos++] = ' ';
    _cube_step[pos++] = 0;

    return _cube_step;

}

uint8_t cube_move_reverse(uint8_t move) {
    uint8_t face = (move & 0x0F);
    uint8_t count = (move & 0xF0) >> 4;
    count = 4 - count;
    return (count << 4) + face;
}

uint8_t cube_move_sum(uint8_t puzzle, uint8_t move1, uint8_t move2) {
    
    uint8_t face1 = move1 & 0x0F;
    uint8_t dir1 = move1 >> 4;
    uint8_t face2 = move2 & 0x0F;
    uint8_t dir2 = move2 >> 4;

    // 1 () + 1 () = 2 (2)
    // 1 () + 3 (') = cancel
    // 1 () + 2 (2) = 3 (')
    // 3 (') + 3 (') = 2 (2)
    // 3 (') + 2 (2) = 1 ()
    // 2 (2) + 2 (2) = cancel

    if (puzzle == PUZZLE_3x3x3) {
        if (face1 != face2) return 0xFF;
    }

    if (puzzle == PUZZLE_2x2x2) {
        if ((face1 % 3) != (face2 % 3)) return 0xFF;
        //if (face1 != face2) dir1 = 4 - dir1;
    }
    
    uint8_t dir = (dir1 + dir2) % 4;
    if (dir == 0) return 0xFE;
    return (dir << 4) + face2;

}

// ----------------------------------------------------------------------------
// Metrics
// ----------------------------------------------------------------------------

void cube_metrics_start(uint32_t ms) {
    _cube_start = (ms == 0) ? _cube_last_move_millis : ms;
    _cube_turns = 0;
    _cube_running_metrics = true;
}

void cube_metrics_end(uint32_t ms) {
    _cube_time = ((ms == 0) ? _cube_last_move_millis : ms) - _cube_start;
    _cube_running_metrics = false;
}

void cube_time(unsigned long ms) {
    _cube_time = ms;
}

uint32_t cube_time() {
    if (_cube_running_metrics) {
        return millis() - _cube_start;
    }
    return _cube_time;
}

bool cube_running_metrics() {
    return _cube_running_metrics;
}
uint16_t cube_turns() {
    return _cube_turns;
}

// ----------------------------------------------------------------------------
// Protected
// Methods called only by smart cubes
// ----------------------------------------------------------------------------

void cube_set_battery(uint8_t battery) {
    _cube_battery = battery;
}

void cube_set_cube_callbacks(void (*_battery)(void), void (*_reset)(void)) {
    _cube_battery_callback = _battery;
    _cube_reset_callback = _reset;
    if (_cube_battery_callback) _cube_battery_callback();
}

bool cube_solved(char * facelet) {
    
    bool solved = false;

    if (g_puzzle == PUZZLE_3x3x3) {
        solved = (strcmp(CUBE_SOLVED_FACELET_3x3x3, facelet) == 0);
    }

    if (solved) {
        if (_cube_callback) _cube_callback(CUBE_EVENT_SOLVED, nullptr);
    }

    return solved;

}

bool cube_solved(uint8_t * corners, uint8_t * edges) {

    bool solved = (memcmp(CUBE_SOLVED_CORNERS, corners, 8) == 0);
    if (g_puzzle == PUZZLE_3x3x3) {
        solved &= (memcmp(CUBE_SOLVED_EDGES, edges, 12) == 0);
    }

    if (solved) {
        if (_cube_callback) _cube_callback(CUBE_EVENT_SOLVED, nullptr);
    }

    return solved;

}

// count is the number of turns clockwise, hence:
// U has count=1
// U2 has count=2
// U' has count=3
void cube_move(uint8_t face, uint8_t count) {

    _cube_last_move_millis = millis();

    uint8_t data[1] = { ((count & 0x0F) << 4) + (face & 0x0F) };
    if (_cube_callback) _cube_callback(CUBE_EVENT_MOVE, data);
    
    // Metrics
    if (_cube_running_metrics) _cube_turns++;

    // Check U turns
    static unsigned long uturns = 0;
    if ((0 == face) && (1 == count)) {
        uturns += 1;
    } else {
        uturns = 0;
    }
    if (uturns == 4) {
        uturns = 0;
        if (_cube_callback) _cube_callback(CUBE_EVENT_4UTURNS, nullptr);
    }

    #if DEBUG > 1
        Serial.print("[CUB] Move: ");
        Serial.print(CUBE_FACES[face]);
        if (2 == count) Serial.print("2");
        if (3 == count) Serial.print("'");
        Serial.println();
    #endif
    
}

void cube_state(char * cubelets, unsigned char len) {

    memcpy(_cube_cubelets, cubelets, len);
    _cube_cubelets[len] = 0;

    #if DEBUG>1
        Serial.printf("[CUB] State: %s\n", _cube_cubelets);
    #endif

    _cube_updated = true;

}

void cube_state(uint8_t * corners, uint8_t * edges, const unsigned char cfacelet[8][3], const unsigned char efacelet[12][2]) {

    #if DEBUG>1
        Serial.printf("[CUB] Corners: ");
        for (uint16_t i=0; i<8; i++) {
            Serial.printf("%d ", corners[i]);
        }
        Serial.println();
        if (g_puzzle == PUZZLE_3x3x3) {
            Serial.printf("[CUB] Edges: ");
            for (uint16_t i=0; i<12; i++) {
                Serial.printf("%d ", edges[i]);
            }
            Serial.println();
        }
    #endif

    if (g_puzzle == PUZZLE_3x3x3) {

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

        _cube_cubelets[54] = 0;

    } else {
        
        for (uint8_t i=0; i<24; i++) {
            _cube_cubelets[i] = CUBE_FACES[int(i / 4)];
        }

        for (uint8_t c=0; c<8; c++) {
            uint8_t j = corners[c] & 0x07;
            uint8_t ori = corners[c] >> 3;
            for (uint8_t n=0; n<3; n++) {
                _cube_cubelets[cfacelet[c][(n + ori) % 3]] = CUBE_FACES[int(cfacelet[j][n] / 4)];
            }
        }
    
        _cube_cubelets[24] = 0;

    }

    #if DEBUG>1
        Serial.printf("[CUB] State: %s\n", _cube_cubelets);
    #endif

    _cube_updated = true;

}


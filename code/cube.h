#ifndef _CUBE_H
#define _CUBE_H

#include "ring.h"

enum {
    CUBE_EVENT_CONNECTED,
    CUBE_EVENT_MOVE,
    CUBE_EVENT_4UTURNS,
    CUBE_EVENT_SOLVED,
    CUBE_EVENT_DISCONNECTED
};

// ------------------------------------

void cube_setup();
void cube_reset();
uint8_t cube_get_battery();
void cube_set_callback(void (*callback)(uint8_t event, uint8_t * data));

void cube_metrics_start(uint32_t ms = 0);
void cube_metrics_end(uint32_t ms = 0);
bool cube_running_metrics();
uint16_t cube_turns();
uint32_t cube_time();
void cube_time(unsigned long ms); // used to set the time from the mat

unsigned char * cube_cubelets();
bool cube_updated();

void cube_scramble(uint8_t puzzle, Ring * moves);
char * cube_turn_text(uint8_t code, bool space = false);
uint8_t cube_move_reverse(uint8_t move);
uint8_t cube_move_sum(uint8_t puzzle, uint8_t move1, uint8_t move2);

// ------------------------------------
// Callbacks from smart cubes
// ------------------------------------
void cube_move(uint8_t face, uint8_t count);
bool cube_solved(char * facelet);
bool cube_solved(uint8_t * corners, uint8_t * edges);
void cube_state(char * cubelets, unsigned char len);
void cube_state(uint8_t * corners, uint8_t * edges, const unsigned char cfacelet[8][3], const unsigned char efacelet[12][2]);
void cube_set_battery(uint8_t battery);
void cube_set_cube_callbacks(void (*)(void), void (*)(void));

// ------------------------------------
// Callbacks from bluetooth module
// ------------------------------------

bool cube_bind(uint8_t conn_handle);
void cube_unbind();

// ------------------------------------

#endif // _CUBE_H
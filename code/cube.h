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

bool cube_solved(uint8_t * corners, uint8_t * edges);
void cube_move(uint8_t face, uint8_t count);
void cube_state(uint8_t * corners, uint8_t * edges, const unsigned char cfacelet[8][3], const unsigned char efacelet[12][2]);

void cube_set_battery(uint8_t battery);
uint8_t cube_get_battery();
void cube_set_callback(void (*callback)(uint8_t event, uint8_t * data));

void cube_setup();
bool cube_bind(uint8_t conn_handle);
void cube_unbind();

void cube_metrics_start();
void cube_metrics_end();
bool cube_running_metrics();
unsigned short cube_turns();
unsigned long cube_time();

unsigned char * cube_cubelets();
bool cube_updated();

void cube_scramble(Ring * moves, uint8_t size);
char * cube_turn_text(uint8_t code);
uint8_t cube_move_reverse(uint8_t move);
uint8_t cube_move_sum(uint8_t move1, uint8_t move2);

#endif // _CUBE_H
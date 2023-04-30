#ifndef _CUBE_H
#define _CUBE_H

bool cube_solved(uint8_t * corners, uint8_t * edges);
void cube_move(uint8_t face, uint8_t dir);
void cube_state(uint8_t * corners, uint8_t * edges, const unsigned char cfacelet[8][3], const unsigned char efacelet[12][2]);

void cube_set_battery(uint8_t battery);
uint8_t cube_get_battery();

void cube_reset();
unsigned char cube_status();
unsigned long cube_time();
unsigned char * cube_cubelets();
bool cube_updated();

#endif // _CUBE_H
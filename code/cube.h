#ifndef _CUBE_H
#define _CUBE_H

bool cube_solved(uint8_t * corners, uint8_t * edges);
void cube_move(uint8_t face, uint8_t dir);
void cube_state(uint8_t * corners, uint8_t * edges, const unsigned char cfacelet[8][3], const unsigned char efacelet[12][2]);

unsigned char cube_status();
unsigned long cube_time();

#endif // _CUBE_H
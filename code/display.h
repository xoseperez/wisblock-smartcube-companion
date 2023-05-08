#ifndef _DISPLAY_H
#define _DISPLAY_H

#include "ring.h"

void display_update_cube(uint16_t center_x = 0, uint16_t center_y = 0, unsigned char size = 20);

void display_page_intro();
void display_page_2d();
void display_page_3d();
void display_page_scramble(Ring * ring);
void display_page_inspect();
void display_page_timer();
void display_page_solved();

void display_on();
void display_off();

void display_start_transaction();
void display_end_transaction();

void display_setup(void);
void display_loop();

#endif // _DISPLAY_H

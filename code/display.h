#ifndef _DISPLAY_H
#define _DISPLAY_H

void display_clear();
void display_show_intro();
void display_show_cube();
void display_setup(void);
void display_update_cube(char * cubelets);
void display_timer(uint8_t min, uint8_t sec, uint16_t ms);
void display_ready();

#endif // _DISPLAY_H

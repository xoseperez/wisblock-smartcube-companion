#ifndef _DISPLAY_H
#define _DISPLAY_H

void display_on();
void display_off();
void display_clear();
void display_show_intro();
void display_show_cube();
void display_setup(void);
void display_loop();
void display_update_cube(char * cubelets);
void display_show_timer();
void display_hide_timer();
void display_show_ready();

#endif // _DISPLAY_H

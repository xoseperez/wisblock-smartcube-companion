#ifndef _DISPLAY_H
#define _DISPLAY_H

#include <Adafruit_ST7789.h>
#include "ring.h"

#define DISPLAY_ALIGN_LEFT      0
#define DISPLAY_ALIGN_CENTER    1
#define DISPLAY_ALIGN_RIGHT     2
#define DISPLAY_ALIGN_TOP       0
#define DISPLAY_ALIGN_MIDDLE    4
#define DISPLAY_ALIGN_BOTTOM    8

#define DISPLAY_WIDTH           320
#define DISPLAY_HEIGHT          240

struct s_button {
	uint8_t id;
    uint16_t x1;
    uint16_t y1;
    uint16_t x2;
    uint16_t y2;
};

// Private
uint16_t display_text(char * text, uint16_t x, uint16_t y, uint8_t align = 0, bool return_x = false);
void display_update_cube(uint16_t center_x = 0, uint16_t center_y = 0, unsigned char size = 20);
void display_button(uint8_t id, char * text, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t buttoncolor, uint32_t textcolor = ST77XX_BLACK);

// Public
void display_page_intro();
void display_page_config(uint8_t mode);
void display_page_smartcube_connect();
void display_page_stackmat_connect();
void display_page_user(uint8_t puzzle, uint8_t user);
void display_page_user_confirm_reset(uint8_t user);
void display_page_2d();
void display_page_3d();
void display_page_scramble(Ring * ring);
void display_page_scramble_manual(Ring * ring);
void display_page_inspect();
void display_page_timer();
void display_page_solved();

void display_clear();
void display_on();
void display_off();

void display_start_transaction();
void display_end_transaction();

void display_setup(void);
void display_loop();

uint8_t display_get_button(uint16_t x, uint16_t y);

extern uint8_t g_user;
extern uint8_t g_state;
extern uint8_t g_mode;

#endif // _DISPLAY_H

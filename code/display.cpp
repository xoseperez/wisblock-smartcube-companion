#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

#include "config.h"
#include "display.h"
#include "bluetooth.h"
#include "cube.h"
#include "utils.h"

#include "assets/bmp_cube.h"

#define DISPLAY_ALIGN_LEFT      0
#define DISPLAY_ALIGN_CENTER    1
#define DISPLAY_ALIGN_RIGHT     2
#define DISPLAY_ALIGN_TOP       0
#define DISPLAY_ALIGN_MIDDLE    4
#define DISPLAY_ALIGN_BOTTOM    8

#define DISPLAY_WIDTH           320
#define DISPLAY_HEIGHT          240

Adafruit_ST7789 _display_screen = Adafruit_ST7789(DISPLAY_CS_GPIO, DISPLAY_DC_GPIO, DISPLAY_RST_GPIO);
GFXcanvas16 _display_canvas = GFXcanvas16(DISPLAY_WIDTH, DISPLAY_HEIGHT);

// URFDLB
uint16_t _cube_colors[6] = { 
    _display_screen.color565(255, 255, 255), 
    _display_screen.color565(255,   0,   0), 
    _display_screen.color565(  0, 144,   0), 
    _display_screen.color565(255, 255,   0), 
    _display_screen.color565(255, 144,  48), 
    _display_screen.color565(  0,   0, 255)
};


// ----------------------------------------------------------------------------
// Private
// ----------------------------------------------------------------------------

static void display_draw_bmp(const GUI_BITMAP *bmp, uint8_t x, uint8_t y) {
    uint32_t index = 0;
    for (uint16_t j=0; j<bmp->ySize; j++) for (uint16_t i=0; i<bmp->xSize; i++) {
        _display_canvas.drawPixel(x+i, y+j, bmp->date[index++]);
    }
}

void display_update_cube(unsigned char size) {

    // Get cubelets
    unsigned char * cubelets = cube_cubelets();

    // translate cubelets to _cube_colors
    uint8_t cc[54] = {0};
    for (uint8_t i=0; i<54; i++) {
      if (cubelets[i] == 'U') cc[i] = 0;
      if (cubelets[i] == 'R') cc[i] = 1;
      if (cubelets[i] == 'F') cc[i] = 2;
      if (cubelets[i] == 'D') cc[i] = 3;
      if (cubelets[i] == 'L') cc[i] = 4;
      if (cubelets[i] == 'B') cc[i] = 5;
    }

    uint8_t gap = size / 10;
    if (gap == 0) gap = 1;
    uint8_t block = (size + gap);
    uint8_t offset_x = (DISPLAY_WIDTH - 12 * block) / 2;
    uint8_t offset_y = (DISPLAY_HEIGHT - 9 * block) / 2;
    uint8_t x = 0, y = 0;
    uint8_t index = 0;

    // U
    x = offset_x + 3 * block;
    y = offset_y + 3 * block;
    for (uint8_t i=0; i<3; i++) for (uint8_t j=0; j<3; j++) {
      _display_canvas.fillRect(x + j * block, y + i * block, size, size, _cube_colors[cc[index++]]);
    }

    // R
    x = offset_x + 6 * block;
    y = offset_y + 3 * block;
    for (uint8_t i=0; i<3; i++) for (uint8_t j=0; j<3; j++) {
      _display_canvas.fillRect(x + i * block, y + (2-j) * block, size, size, _cube_colors[cc[index++]]);
    }

    // F
    x = offset_x + 3 * block;
    y = offset_y + 6 * block;
    for (uint8_t i=0; i<3; i++) for (uint8_t j=0; j<3; j++) {
      _display_canvas.fillRect(x + j * block, y + i * block, size, size, _cube_colors[cc[index++]]);
    }

    // D
    x = offset_x + 9 * block;
    y = offset_y + 3 * block;
    for (uint8_t i=0; i<3; i++) for (uint8_t j=0; j<3; j++) {
      _display_canvas.fillRect(x + (2-j) * block, y + (2-i) * block, size, size, _cube_colors[cc[index++]]);
    }

    // L
    x = offset_x + 0 * block;
    y = offset_y + 3 * block;
    for (uint8_t i=0; i<3; i++) for (uint8_t j=0; j<3; j++) {
      _display_canvas.fillRect(x + (2-i) * block, y + j * block, size, size, _cube_colors[cc[index++]]);
    }

    // B
    x = offset_x + 3 * block;
    y = offset_y + 0 * block;
    for (uint8_t i=0; i<3; i++) for (uint8_t j=0; j<3; j++) {
      _display_canvas.fillRect(x + (2-j) * block, y + (2-i) * block, size, size, _cube_colors[cc[index++]]);
    }

}

void display_text(char * text, uint16_t x, uint16_t y, uint8_t align) {

    // Get text dimensions
    int16_t a, b;
    uint16_t w, h;
    _display_canvas.getTextBounds(text, 0, 0, &a, &b, &w, &h);

    // Calculate position
    if (align & DISPLAY_ALIGN_CENTER) x = x - (w / 2);
    if (align & DISPLAY_ALIGN_RIGHT) x = x - w;
    if (align & DISPLAY_ALIGN_MIDDLE) y = y - (h / 2);
    if (align & DISPLAY_ALIGN_BOTTOM) y = y - h;

    // Display
    _display_canvas.setCursor(x, y);
    _display_canvas.println(text);    

}

// ----------------------------------------------------------------------------
// Private
// ----------------------------------------------------------------------------

void display_clear() {
    _display_screen.fillScreen(ST77XX_BLACK);
}

void display_battery() {
    
    uint8_t battery;
    char buffer[10] = {0};
    
    battery = utils_get_battery();
    sprintf(buffer, "BAT %02d%%", battery);
    if (battery > 25) {
        _display_canvas.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    } else {
        _display_canvas.setTextColor(ST77XX_RED, ST77XX_BLACK);
    }
    _display_canvas.setTextSize(1);
    display_text(buffer, 310, 10, DISPLAY_ALIGN_RIGHT);

    battery = cube_get_battery();
    if (battery != 0xFF) {
        sprintf(buffer, "CUBE %02d%%", battery);
        if (battery > 25) {
            _display_canvas.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
        } else {
            _display_canvas.setTextColor(ST77XX_RED, ST77XX_BLACK);
        }
        _display_canvas.setTextSize(1);
        display_text(buffer, 310, 20, DISPLAY_ALIGN_RIGHT);
    }

}

void display_show_intro() {
    
    display_clear();
    display_draw_bmp(&bmp_cube_info, 20, 20);
    _display_canvas.setTextSize(0);
    _display_canvas.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    display_text((char *) APP_NAME, 310, 210, DISPLAY_ALIGN_RIGHT | DISPLAY_ALIGN_BOTTOM);
    display_text((char *) APP_VERSION, 310, 220, DISPLAY_ALIGN_RIGHT | DISPLAY_ALIGN_BOTTOM);
    _display_canvas.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
    display_text((char *) "Connect cube...", 310, 230, DISPLAY_ALIGN_RIGHT | DISPLAY_ALIGN_BOTTOM);
    display_battery();

}

void display_hide_timer() {
    _display_canvas.fillRect(160, 180, 150, 40, ST77XX_BLACK);
}

void display_show_timer() {

    unsigned long time = cube_time();
    unsigned short turns = cube_turns();
    unsigned char status = cube_status();
    char buffer[30] = {0};

    unsigned long tmp = time;
    uint16_t ms = tmp % 1000;
    tmp /= 1000;
    uint8_t sec = tmp % 60;
    tmp /= 60;
    uint8_t min = tmp;
    sprintf(buffer, "%02d:%02d.%03d", min, sec, ms);


    _display_canvas.setTextColor((3 == status) ? ST77XX_WHITE : ST77XX_GREEN, ST77XX_BLACK);
    _display_canvas.setTextSize(2);
    display_text(buffer, 226, 180, DISPLAY_ALIGN_CENTER);

    if (3 == status) {
        sprintf(buffer, "TURNS %d", turns);
    } else {
        float tps = turns / ( time / 1000.0 );
        sprintf(buffer, "TURNS %d | TPS %2.1f", turns, tps);
    }
    
    _display_canvas.setTextSize(1);
    display_text(buffer, 226, 200, DISPLAY_ALIGN_CENTER);

}

void display_show_ready() {
    _display_canvas.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
    _display_canvas.setTextSize(2);
    display_text((char *) "READY...  ", 175, 180, 0);
}

void display_off() {
    digitalWrite(DISPLAY_BL_GPIO, LOW);
    digitalWrite(WB_IO2, LOW);
}

void display_on() {
    digitalWrite(WB_IO2, HIGH);
    digitalWrite(DISPLAY_BL_GPIO, HIGH);
}

void display_start_transaction() {
    _display_canvas.fillScreen(ST77XX_BLACK);
}

void display_end_transaction() {
    _display_screen.drawRGBBitmap(0, 0, _display_canvas.getBuffer(), DISPLAY_WIDTH, DISPLAY_HEIGHT);
}

void display_setup(void) {
  
    pinMode(WB_IO2, OUTPUT);
    pinMode(DISPLAY_BL_GPIO, OUTPUT);

    display_on();

    _display_screen.init(DISPLAY_HEIGHT, DISPLAY_WIDTH); // reversed
    _display_screen.setRotation(3);

    #if DEBUG > 0
        Serial.printf("[TFT] Initialized\n");
    #endif

}

void display_loop() {

}

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

#include "config.h"
#include "display.h"
#include "bluetooth.h"
#include "cube.h"
#include "utils.h"
#include "bmp_cube.h"

#define DISPLAY_ALIGN_LEFT    0
#define DISPLAY_ALIGN_CENTER  1
#define DISPLAY_ALIGN_RIGHT   2
#define DISPLAY_ALIGN_TOP     0
#define DISPLAY_ALIGN_MIDDLE  4
#define DISPLAY_ALIGN_BOTTOM  8

Adafruit_ST7789 display_tft = Adafruit_ST7789(DISPLAY_CS_GPIO, DISPLAY_DC_GPIO, DISPLAY_RST_GPIO);

// ----------------------------------------------------------------------------
// Private
// ----------------------------------------------------------------------------

static void display_draw_bmp(const GUI_BITMAP *bmp , uint8_t x, uint8_t y) {
    
    uint16_t color = bmp->date[0];
    uint32_t count = 0;
    uint64_t bufSize = bmp->xSize * bmp->ySize;
    display_tft.startWrite();
    display_tft.setAddrWindow(x, y, bmp->xSize, bmp->ySize);

    for ( uint64_t i = 0 ; i < bufSize ; i++ ) {
        if(color == bmp->date[i]) {
            count++;
        } else {
            display_tft.writeColor(color, count); 
            count = 1;
            color = bmp->date[i];
        }
    }

    display_tft.writeColor(color, count); 
    display_tft.endWrite();

}

void display_update_cube() {

    // Get cubelets
    unsigned char * cubelets = cube_cubelets();

    // URFDLB
    uint16_t colors[6] = { 
      display_tft.color565(255, 255, 255), 
      display_tft.color565(255,   0,   0), 
      display_tft.color565(  0, 144,   0), 
      display_tft.color565(255, 255,   0), 
      display_tft.color565(255, 144,  48), 
      display_tft.color565(  0,   0, 255)
    };
    
    // translate cubelets to colors
    uint8_t cc[54] = {0};
    for (uint8_t i=0; i<54; i++) {
      if (cubelets[i] == 'U') cc[i] = 0;
      if (cubelets[i] == 'R') cc[i] = 1;
      if (cubelets[i] == 'F') cc[i] = 2;
      if (cubelets[i] == 'D') cc[i] = 3;
      if (cubelets[i] == 'L') cc[i] = 4;
      if (cubelets[i] == 'B') cc[i] = 5;
    }

    // 240x320
    uint8_t gap = 2;
    uint8_t size = 20;
    uint8_t block = (size + gap);
    uint8_t offset_x = (320 - 12 * block) / 2;
    uint8_t offset_y = (240 - 9 * block) / 2;
    uint8_t x = 0, y = 0;
    uint8_t index = 0;

    // U
    x = offset_x + 3 * block;
    y = offset_y + 3 * block;
    for (uint8_t i=0; i<3; i++) for (uint8_t j=0; j<3; j++) {
      display_tft.fillRect(x + j * block, y + i * block, size, size, colors[cc[index++]]);
    }

    // R
    x = offset_x + 6 * block;
    y = offset_y + 3 * block;
    for (uint8_t i=0; i<3; i++) for (uint8_t j=0; j<3; j++) {
      display_tft.fillRect(x + i * block, y + (2-j) * block, size, size, colors[cc[index++]]);
    }

    // F
    x = offset_x + 3 * block;
    y = offset_y + 6 * block;
    for (uint8_t i=0; i<3; i++) for (uint8_t j=0; j<3; j++) {
      display_tft.fillRect(x + j * block, y + i * block, size, size, colors[cc[index++]]);
    }

    // D
    x = offset_x + 9 * block;
    y = offset_y + 3 * block;
    for (uint8_t i=0; i<3; i++) for (uint8_t j=0; j<3; j++) {
      display_tft.fillRect(x + (2-j) * block, y + (2-i) * block, size, size, colors[cc[index++]]);
    }

    // L
    x = offset_x + 0 * block;
    y = offset_y + 3 * block;
    for (uint8_t i=0; i<3; i++) for (uint8_t j=0; j<3; j++) {
      display_tft.fillRect(x + (2-i) * block, y + j * block, size, size, colors[cc[index++]]);
    }

    // B
    x = offset_x + 3 * block;
    y = offset_y + 0 * block;
    for (uint8_t i=0; i<3; i++) for (uint8_t j=0; j<3; j++) {
      display_tft.fillRect(x + (2-j) * block, y + (2-i) * block, size, size, colors[cc[index++]]);
    }

}

void display_text(char * text, uint16_t x, uint16_t y, uint8_t align) {

    // Get text dimensions
    int16_t a, b;
    uint16_t w, h;
    display_tft.getTextBounds(text, 0, 0, &a, &b, &w, &h);

    // Calculate position
    if (align & DISPLAY_ALIGN_CENTER) x = x - (w / 2);
    if (align & DISPLAY_ALIGN_RIGHT) x = x - w;
    if (align & DISPLAY_ALIGN_MIDDLE) y = y - (h / 2);
    if (align & DISPLAY_ALIGN_BOTTOM) y = y - h;

    // Display
    display_tft.setCursor(x, y);
    display_tft.println(text);    

}

// ----------------------------------------------------------------------------
// Private
// ----------------------------------------------------------------------------

void display_clear() {
    display_tft.fillScreen(ST77XX_BLACK);
}

void display_battery() {
    
    uint8_t battery;
    char buffer[10] = {0};
    
    battery = utils_get_battery();
    sprintf(buffer, "BAT %02d%%", battery);
    if (battery > 25) {
        display_tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    } else {
        display_tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
    }
    display_tft.setTextSize(1);
    display_text(buffer, 310, 10, DISPLAY_ALIGN_RIGHT);

    battery = cube_get_battery();
    if (battery != 0xFF) {
        sprintf(buffer, "CUBE %02d%%", battery);
        if (battery > 25) {
            display_tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
        } else {
            display_tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
        }
        display_tft.setTextSize(1);
        display_text(buffer, 310, 20, DISPLAY_ALIGN_RIGHT);
    }

}

void display_show_intro() {
    
    display_clear();
    display_draw_bmp(&bmp_cube_info, 20, 20);
    display_tft.setTextSize(0);
    display_tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    display_text((char *) APP_NAME, 310, 210, DISPLAY_ALIGN_RIGHT | DISPLAY_ALIGN_BOTTOM);
    display_text((char *) APP_VERSION, 310, 220, DISPLAY_ALIGN_RIGHT | DISPLAY_ALIGN_BOTTOM);
    display_tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
    display_text((char *) "Connect cube...", 310, 230, DISPLAY_ALIGN_RIGHT | DISPLAY_ALIGN_BOTTOM);
    display_battery();

}

void display_hide_timer() {
    display_tft.fillRect(160, 180, 150, 40, ST77XX_BLACK);
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


    display_tft.setTextColor((3 == status) ? ST77XX_WHITE : ST77XX_GREEN, ST77XX_BLACK);
    display_tft.setTextSize(2);
    display_text(buffer, 226, 180, DISPLAY_ALIGN_CENTER);

    if (3 == status) {
        sprintf(buffer, "TURNS %d", turns);
    } else {
        float tps = turns / ( time / 1000.0 );
        sprintf(buffer, "TURNS %d | TPS %2.1f", turns, tps);
    }
    
    display_tft.setTextSize(1);
    display_text(buffer, 226, 200, DISPLAY_ALIGN_CENTER);

}

void display_show_ready() {
    display_tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
    display_tft.setTextSize(2);
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

void display_setup(void) {
  
    pinMode(WB_IO2, OUTPUT);
    pinMode(DISPLAY_BL_GPIO, OUTPUT);

    display_on();

    display_tft.init(240, 320);
    display_tft.setRotation(3);

    #if DEBUG > 0
        Serial.printf("[TFT] Initialized\n");
    #endif

    display_clear();
    display_show_intro();

}

void display_loop() {
    
    static unsigned long last = 0;
    if (millis() - last < 10) return;
    last = millis();

    if (cube_updated()) {
      display_update_cube();
      display_battery();
    }

    static unsigned char old_status = 0xFF;
    unsigned char status = cube_status();
    if (2 == status) {
        if (2 != old_status) {
            display_hide_timer();
            display_show_ready();
        }
    } else if ((1 == status) || (3 == status)) {
        display_show_timer();
    }
    old_status = status;

}

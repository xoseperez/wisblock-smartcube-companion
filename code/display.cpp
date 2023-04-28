#include <Arduino.h>

#include "config.h"
#include "display.h"
#include "cube.h"

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

#include "bmp_cube.h"

#define CS            SS
#define BL            WB_IO3
#define RST           WB_IO5
#define DC            WB_IO4

Adafruit_ST7789 display_tft = Adafruit_ST7789(CS, DC, RST);

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

void display_update_cube(char * cubelets) {

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

// ----------------------------------------------------------------------------
// Private
// ----------------------------------------------------------------------------

void display_clear() {
    display_tft.fillScreen(ST77XX_BLACK);
}

void display_show_intro() {
    display_clear();
    display_draw_bmp(&bmp_cube_info, 60, 20);
    display_tft.setCursor(230, 220);
    display_tft.setTextColor(ST77XX_WHITE);
    display_tft.setTextSize(0);
    display_tft.println("APPROACH CUBE");    
}

void display_show_timer() {

    char buffer[11];
    uint32_t time = cube_time();
    uint16_t ms = time % 1000;
    time /= 1000;
    uint8_t sec = time % 60;
    time /= 60;
    uint8_t min = time;
    sprintf(buffer, "%02d:%02d.%03d", min, sec, ms);

    display_tft.setCursor(175, 180);
    if (cube_status() == 2) {
        display_tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    } else {
        display_tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
    }
    display_tft.setTextSize(2);
    display_tft.println(buffer);    

}

void display_show_ready() {
    display_tft.setCursor(175, 180);
    display_tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
    display_tft.setTextSize(2);
    display_tft.println("READY...  ");    
}

void display_setup(void) {
  
    pinMode(WB_IO2, OUTPUT);
    digitalWrite(WB_IO2, HIGH);
  
    pinMode(BL, OUTPUT);
    digitalWrite(BL, HIGH); // Enable the backlight, you can also adjust the backlight brightness through PWM.

    display_tft.init(240, 320); // Init ST7789 240x240.
    display_tft.setRotation(1);

    #if DEBUG > 0
        Serial.printf("[TFT] Initialized\n");
    #endif

    display_clear();
    display_show_intro();

}

void display_loop() {
    
    if (cube_status() == 2) {

        static unsigned long last = 0;
        if (millis() - last > 10) {
            last = millis();
            display_show_timer();
        }

    }
}

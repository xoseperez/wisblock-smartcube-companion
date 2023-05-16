#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

#include "config.h"
#include "display.h"
#include "bluetooth.h"
#include "cube.h"
#include "utils.h"
#include "flash.h"
#include "ring.h"

#include "assets/bmp_cube.h"

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

s_button _display_buttons[10];
uint8_t _display_buttons_count = 0;

const char * DISPLAY_CONFIG_BUTTONS[] = {
    "SMARTCUBE",
    "STACKMAT",
    "MANUAL",
    "SHUT DOWN"
};


// ----------------------------------------------------------------------------
// Private
// ----------------------------------------------------------------------------

static void display_draw_bmp(const GUI_BITMAP *bmp, uint16_t x, uint16_t y, uint8_t step) {
    uint32_t index = 0;
    uint16_t w = bmp->xSize / step;
    uint16_t h = bmp->ySize / step;
    for (uint16_t j=0; j<h; j++) {
        for (uint16_t i=0; i<w; i++) {
            _display_canvas.drawPixel(x+i, y+j, bmp->date[index]);
            index += step;
        }
        index += (w * (step - 1));
    }
}

int16_t display_points[27][4][3] = {0};
float display_matrix[3][3] = {0};
float display_angle_alpha = 150.0 * PI / 180.0; // z axis
float display_angle_beta = 180.0 * PI / 180.0; // y axis
float display_angle_gamma = 20.0 * PI / 180.0; // x axis

void display_update_cube_3d_init() {

    uint8_t index = 0;
    int16_t x, y, z;
    int16_t size = 40;
    int16_t gap = 8;
    int16_t start = 1.5 * size + gap;
    
    // white face
    x = -start;
    y = -start;
    z = start + gap / 2;
    for (uint8_t i=0; i<3; i++) for (uint8_t j=0; j<3; j++) {
        for (uint8_t k=0; k<4; k++) {
            display_points[index][k][0] = x + j * (size + gap) + ((k == 1 || k == 2) ? size : 0);
            display_points[index][k][1] = y + i * (size + gap) + ((k == 2 || k == 3) ? size : 0);
            display_points[index][k][2] = z;
        }
        index++;
    }

    // red face
    x = start + gap / 2;
    y = start;
    z = start;
    for (uint8_t i=0; i<3; i++) for (uint8_t j=0; j<3; j++) {
        for (uint8_t k=0; k<4; k++) {
            display_points[index][k][0] = x;
            display_points[index][k][1] = y - j * (size + gap) - ((k == 1 || k == 2) ? size : 0);
            display_points[index][k][2] = z - i * (size + gap) - ((k == 2 || k == 3) ? size : 0);
        }
        index++;
    }

    // green face
    x = -start;
    y = start + gap / 2;
    z = start;
    for (uint8_t i=0; i<3; i++) for (uint8_t j=0; j<3; j++) {
        for (uint8_t k=0; k<4; k++) {
            display_points[index][k][0] = x + j * (size + gap) + ((k == 1 || k == 2) ? size : 0);
            display_points[index][k][1] = y;
            display_points[index][k][2] = z - i * (size + gap) - ((k == 2 || k == 3) ? size : 0);
        }
        index++;
    }

    // Calculate angles
    float sin_alpha = sin(display_angle_alpha);
    float cos_alpha = cos(display_angle_alpha);
    float sin_beta  = sin(display_angle_beta);
    float cos_beta  = cos(display_angle_beta);
    float sin_gamma = sin(display_angle_gamma);
    float cos_gamma = cos(display_angle_gamma);

    // Define matrix
    display_matrix[0][0] = cos_alpha * cos_beta;
    display_matrix[0][1] = cos_alpha * sin_beta * sin_gamma - sin_alpha * cos_gamma;
    display_matrix[0][2] = cos_alpha * sin_beta * cos_gamma + sin_alpha * sin_gamma;
    display_matrix[1][0] = sin_alpha * cos_beta;
    display_matrix[1][1] = sin_alpha * sin_beta * sin_gamma + cos_alpha * cos_gamma;
    display_matrix[1][2] = sin_alpha * sin_beta * cos_gamma - cos_alpha * sin_gamma;
    display_matrix[2][0] = -sin_beta;
    display_matrix[2][1] = cos_beta * sin_gamma;
    display_matrix[2][2] = cos_beta * cos_gamma;

}

void display_update_cube_3d() {

    // Get cubelets
    unsigned char * cubelets = cube_cubelets();

    // translate cubelets to _cube_colors
    uint8_t cc[27] = {0};
    for (uint8_t i=0; i<27; i++) {
      if (cubelets[i] == 'U') cc[i] = 0;
      if (cubelets[i] == 'R') cc[i] = 1;
      if (cubelets[i] == 'F') cc[i] = 2;
      if (cubelets[i] == 'D') cc[i] = 3;
      if (cubelets[i] == 'L') cc[i] = 4;
      if (cubelets[i] == 'B') cc[i] = 5;
    }

    int16_t points[4][3];
    int16_t offset[3] = { DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, DISPLAY_HEIGHT / 2};

    for (uint8_t cubelet=0; cubelet<27; cubelet++) {
        
        // Rotate points
        for (uint8_t vertex=0; vertex<4; vertex++) {
            for (uint8_t coordinate=0; coordinate<3; coordinate++) {
                points[vertex][coordinate] = 
                    display_matrix[0][coordinate] * display_points[cubelet][vertex][0] +
                    display_matrix[1][coordinate] * display_points[cubelet][vertex][1] +
                    display_matrix[2][coordinate] * display_points[cubelet][vertex][2] +
                    offset[coordinate];
            }
        }

        // Show cubelet
        _display_canvas.fillTriangle(points[0][0], points[0][2], points[1][0], points[1][2], points[2][0], points[2][2], _cube_colors[cc[cubelet]]);
        _display_canvas.fillTriangle(points[0][0], points[0][2], points[2][0], points[2][2], points[3][0], points[3][2], _cube_colors[cc[cubelet]]);

    }

}


void display_update_cube(uint16_t center_x, uint16_t center_y, unsigned char size) {

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
    if (center_x == 0) center_x = DISPLAY_WIDTH / 2;
    if (center_y == 0) center_y = DISPLAY_HEIGHT / 2;
    uint8_t block = (size + gap);
    uint8_t offset_x = center_x - 12 * block / 2;
    uint8_t offset_y = center_y - 9 * block / 2;
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

uint16_t display_text_width(char * text) {
    int16_t a, b;
    uint16_t w, h;
    _display_canvas.getTextBounds(text, 0, 0, &a, &b, &w, &h);
    return w;
}

uint16_t display_text_height(char * text) {
    int16_t a, b;
    uint16_t w, h;
    _display_canvas.getTextBounds(text, 0, 0, &a, &b, &w, &h);
    return h;
}

uint16_t display_text(char * text, uint16_t x, uint16_t y, uint8_t align, bool return_x) {

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

    if (return_x) {
        return x + w;
    } else {
        return y + h;
    }

}

void display_stats() {
    
    uint8_t y=0;
    uint8_t battery;
    char buffer[10] = {0};
    
    _display_canvas.setTextSize(1);

    sprintf(buffer, "USER %d", g_user+1);
    _display_canvas.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
    display_text(buffer, 310, y+=10, DISPLAY_ALIGN_RIGHT);

    if (g_mode < 3) {
        _display_canvas.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
        display_text((char *) DISPLAY_CONFIG_BUTTONS[g_mode], 310, y+=10, DISPLAY_ALIGN_RIGHT);
    }

    battery = utils_get_battery();
    sprintf(buffer, "BAT %02d%%", battery);
    if (battery > 25) {
        _display_canvas.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    } else {
        _display_canvas.setTextColor(ST77XX_RED, ST77XX_BLACK);
    }
    display_text(buffer, 310, y+=10, DISPLAY_ALIGN_RIGHT);

    battery = cube_get_battery();
    if (battery != 0xFF) {
        sprintf(buffer, "CUBE %02d%%", battery);
        if (battery > 25) {
            _display_canvas.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
        } else {
            _display_canvas.setTextColor(ST77XX_RED, ST77XX_BLACK);
        }
        display_text(buffer, 310, y+=10, DISPLAY_ALIGN_RIGHT);
    }

}

void display_show_timer() {

    unsigned long time = cube_time();
    unsigned short turns = cube_turns();
    float tps = utils_tps(time, turns);
    char buffer[30] = {0};

    utils_time_to_text(time, buffer, true);
    _display_canvas.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
    _display_canvas.setTextSize(4);
    display_text(buffer, DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2-10, DISPLAY_ALIGN_CENTER | DISPLAY_ALIGN_MIDDLE);
     
    if (g_mode == 0) {
        if (cube_running_metrics()) {
            sprintf(buffer, "TURNS %d", turns);
        } else {
            sprintf(buffer, "TURNS %d | TPS %.2f", turns, tps);
        }
        _display_canvas.setTextSize(1);
        display_text(buffer, DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2+20, DISPLAY_ALIGN_CENTER | DISPLAY_ALIGN_MIDDLE);
    }

}

// ----------------------------------------------------------------------------
// Buttons
// ----------------------------------------------------------------------------

void display_clear_buttons() {
    _display_buttons_count = 0;
}

void display_button(uint8_t id, char * text, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t buttoncolor, uint32_t textcolor) {

    // Draw button
    _display_canvas.fillRoundRect(x, y, w, h, 5, buttoncolor);
    _display_canvas.setTextColor(textcolor, buttoncolor);
    display_text(text, x + w / 2, y + h / 2, DISPLAY_ALIGN_CENTER | DISPLAY_ALIGN_MIDDLE);

    // Register button
    if (_display_buttons_count<10) {
        _display_buttons[_display_buttons_count].id = id;
        _display_buttons[_display_buttons_count].x1 = x;
        _display_buttons[_display_buttons_count].y1 = y;
        _display_buttons[_display_buttons_count].x2 = x+w;
        _display_buttons[_display_buttons_count].y2 = y+h;
        _display_buttons_count++;
    }

}

uint8_t display_get_button(uint16_t x, uint16_t y) {
    for (uint8_t i=0; i<_display_buttons_count; i++) {
        if ((_display_buttons[i].x1 <= x) && (x <= _display_buttons[i].x2)) {
            if ((_display_buttons[i].y1 <= y) && (y <= _display_buttons[i].y2)) {
                return i;
            }
        }
    }
    return 0xFF;
}

// ----------------------------------------------------------------------------
// Public
// ----------------------------------------------------------------------------

void display_page_intro() {
    
    display_draw_bmp(&bmp_cube_info, 20, 20, 1);
    _display_canvas.setTextSize(1);
    _display_canvas.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    display_text((char *) APP_NAME, 310, 210, DISPLAY_ALIGN_RIGHT | DISPLAY_ALIGN_BOTTOM);
    display_text((char *) APP_VERSION, 310, 220, DISPLAY_ALIGN_RIGHT | DISPLAY_ALIGN_BOTTOM);
    display_stats();

}

void display_page_config(uint8_t mode) {

    uint16_t margin = 10;
    uint16_t height = (DISPLAY_HEIGHT - 5 * margin) / 4;

    display_clear_buttons();
    _display_canvas.setTextSize(2);
    for (uint8_t i=0; i<4; i++) {
        display_button(i, (char *) DISPLAY_CONFIG_BUTTONS[i], margin, margin + (height + margin) * i, DISPLAY_WIDTH - 2 * margin, height, mode == i ? ST77XX_GREEN : ST77XX_RED);
    }

}

void display_page_smartcube_connect() {
    
    display_draw_bmp(&bmp_cube_info, 160-50, 120-50, 2);

    _display_canvas.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
    _display_canvas.setTextSize(2);
    display_text((char *) "PAIRING...", 160, 40, DISPLAY_ALIGN_CENTER | DISPLAY_ALIGN_MIDDLE);
    _display_canvas.setTextSize(1);
    display_text((char *) "Bring the cube closer", 160, 200, DISPLAY_ALIGN_CENTER | DISPLAY_ALIGN_MIDDLE);

}

void display_page_stackmat_connect() {
    
    display_draw_bmp(&bmp_cube_info, 160-50, 120-50, 2);

    _display_canvas.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
    _display_canvas.setTextSize(2);
    display_text((char *) "LISTENING...", 160, 40, DISPLAY_ALIGN_CENTER | DISPLAY_ALIGN_MIDDLE);
    _display_canvas.setTextSize(1);
    display_text((char *) "Connect and turn on the StackMat", 160, 200, DISPLAY_ALIGN_CENTER | DISPLAY_ALIGN_MIDDLE);

}

void display_page_2d() {
    display_update_cube();
    display_stats();
}

void display_page_3d() {
    display_update_cube_3d();
    display_stats();
}

void display_page_user(uint8_t user) {

    uint8_t y=20;
    uint8_t step_y=10;
    uint8_t x=35;
    char line[60] = {0};
    char buffer[30] = {0};

    _display_canvas.setTextSize(2);
    _display_canvas.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
    snprintf(line, sizeof(line), "STATS FOR USER %d", user+1);
    display_text(line, 100+x, y, DISPLAY_ALIGN_CENTER | DISPLAY_ALIGN_TOP);
    y+=20;

    _display_canvas.setTextSize(1);
    _display_canvas.setTextColor(ST77XX_WHITE, ST77XX_BLACK);

    // Header
    snprintf(line, sizeof(line), "            TIME   TURNS     TPS");
    display_text(line, x, y+=step_y, DISPLAY_ALIGN_LEFT | DISPLAY_ALIGN_TOP);
    snprintf(line, sizeof(line), "----- ---------- ------- -------");
    display_text(line, x, y+=step_y, DISPLAY_ALIGN_LEFT | DISPLAY_ALIGN_TOP);

    // Best
    _display_canvas.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
    snprintf(line, sizeof(line), "BEST   %s           %5.2f", utils_time_to_text(g_settings.user[user].best.time, buffer, false), g_settings.user[user].best.tps / 100.0);
    display_text(line, x, y+=step_y, DISPLAY_ALIGN_LEFT | DISPLAY_ALIGN_TOP);

    // AV5 & AV12
    _display_canvas.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
    snprintf(line, sizeof(line), " AV5   %s    %4d   %5.2f", utils_time_to_text(g_settings.user[user].av5.time, buffer, false), g_settings.user[user].av5.turns, g_settings.user[user].av5.tps / 100.0);
    display_text(line, x, y+=step_y, DISPLAY_ALIGN_LEFT | DISPLAY_ALIGN_TOP);
    snprintf(line, sizeof(line), "AV12   %s    %4d   %5.2f", utils_time_to_text(g_settings.user[user].av12.time, buffer, false), g_settings.user[user].av12.turns, g_settings.user[user].av12.tps / 100.0);
    display_text(line, x, y+=step_y, DISPLAY_ALIGN_LEFT | DISPLAY_ALIGN_TOP);

    // Last 5
    _display_canvas.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    for (uint8_t i=0; i<12; i++) {
        snprintf(line, sizeof(line), "%4d   %s    %4d   %5.2f", i+1, utils_time_to_text(g_settings.user[user].solve[i].time, buffer, false), g_settings.user[user].solve[i].turns, g_settings.user[user].solve[i].tps / 100.0);
        display_text(line, x, y+=step_y, DISPLAY_ALIGN_LEFT | DISPLAY_ALIGN_TOP);
    }

    // Show buttons
    uint16_t margin = 10;
    uint16_t size = (DISPLAY_HEIGHT - 5 * margin) / 4;
    _display_canvas.setTextSize(2);
    display_clear_buttons();
    for (uint8_t i=0; i<4; i++) {
        snprintf(line, sizeof(line), "%d", i+1);
        display_button(i, line, DISPLAY_WIDTH - margin - size, margin + (size + margin) * i, size, size, user == i ? ST77XX_GREEN : ST77XX_RED);
    }


}

void display_page_user_confirm_reset(uint8_t user) {

    char line[60] = {0};

    // Question
    _display_canvas.setTextSize(2);
    _display_canvas.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
    snprintf(line, sizeof(line), "RESET STATS FOR USER %d?", user+1);
    display_text(line, 160, 80, DISPLAY_ALIGN_CENTER | DISPLAY_ALIGN_MIDDLE);

    uint8_t button_width = 100;
    uint8_t button_height = 40;
    uint8_t button_separation = 20;

    // Buttons
    display_clear_buttons();
    display_button(0, (char *) "YES", (DISPLAY_WIDTH - button_separation) / 2 - button_width, 120, button_width, button_height, ST77XX_RED);
    display_button(1, (char *) "NO", (DISPLAY_WIDTH + button_separation) / 2, 120, button_width, button_height, ST77XX_GREEN);

}

void display_page_scramble(Ring * ring) {

    char buffer[20];

    _display_canvas.setTextColor(ST77XX_GREEN, ST77XX_BLACK);

    _display_canvas.setTextSize(2);
    display_text((char *) "SCRAMBLE", DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2-50, DISPLAY_ALIGN_CENTER | DISPLAY_ALIGN_MIDDLE);

    uint8_t move = ring->peek();
    char * text = cube_turn_text(move);
    _display_canvas.setTextSize(6);
    display_text(text, DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2, DISPLAY_ALIGN_CENTER | DISPLAY_ALIGN_MIDDLE);

    _display_canvas.setTextSize(1);
    sprintf(buffer, "%d TURNS TO GO", ring->available());
    display_text(buffer, DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2+40, DISPLAY_ALIGN_CENTER | DISPLAY_ALIGN_MIDDLE);

    display_stats();

}

void display_page_scramble_manual(Ring * ring) {

    uint16_t x = 10;
    uint16_t y = 10;
    uint16_t h;

    _display_canvas.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
    _display_canvas.setTextSize(2);
    y = display_text((char *) "SCRAMBLE", x, y);

    _display_canvas.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
    _display_canvas.setTextSize(3);
    h = display_text_height((char *) "TEST") + 5;
    y += h;

    while (ring->available()) {
    
        uint8_t move = ring->read();
        char * text = cube_turn_text(move, true);

        if (x + display_text_width(text) > DISPLAY_WIDTH) {
            x = 10;
            y += h;
        }

        x = display_text(text, x, y, DISPLAY_ALIGN_LEFT | DISPLAY_ALIGN_TOP, true);

    }

    // Buttons
    display_clear_buttons();
    _display_canvas.setTextSize(2);
    display_button(0, (char *) "DONE", (DISPLAY_WIDTH - 100) / 2, 180, 100, 40, ST77XX_GREEN);

    display_stats();

}

void display_page_inspect() {
    _display_canvas.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
    _display_canvas.setTextSize(6);
    display_text((char *) "INSPECT", DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2-10, DISPLAY_ALIGN_CENTER | DISPLAY_ALIGN_MIDDLE);
    _display_canvas.setTextSize(1);
    display_text((char *) "Timer will start with first move", DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2+30, DISPLAY_ALIGN_CENTER | DISPLAY_ALIGN_MIDDLE);
    display_stats();
}

void display_page_timer() {
    display_show_timer();
    display_stats();
}

void display_page_solved() {
    display_show_timer();
    display_stats();
}

// ----------------------------------------------------------------------------

void display_clear() {
    _display_screen.fillScreen(ST77XX_BLACK);
}

void display_off() {
    digitalWrite(DISPLAY_BL_GPIO, LOW);
    digitalWrite(DISPLAY_EN_GPIO, LOW);
}

void display_on() {
    digitalWrite(DISPLAY_EN_GPIO, HIGH);
    digitalWrite(DISPLAY_BL_GPIO, HIGH);
}

// ----------------------------------------------------------------------------

void display_start_transaction() {
    _display_canvas.fillScreen(ST77XX_BLACK);
}

void display_end_transaction() {
    _display_screen.drawRGBBitmap(0, 0, _display_canvas.getBuffer(), DISPLAY_WIDTH, DISPLAY_HEIGHT);
}

// ----------------------------------------------------------------------------

void display_setup(void) {
  
    pinMode(DISPLAY_EN_GPIO, OUTPUT);
    pinMode(DISPLAY_BL_GPIO, OUTPUT);

    display_on();

    _display_screen.init(DISPLAY_HEIGHT, DISPLAY_WIDTH); // reversed
    _display_screen.setRotation(3);

    display_update_cube_3d_init();

    #if DEBUG > 0
        Serial.printf("[TFT] Initialized\n");
    #endif

}

void display_loop() {

}

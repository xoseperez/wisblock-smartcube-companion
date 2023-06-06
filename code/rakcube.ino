#include <Arduino.h>
#include <SPI.h>

#include "config.h"
#include "bluetooth.h"
#include "utils.h"
#include "display.h"
#include "touch.h"
#include "cube.h"
#include "stackmat.h"
#include "flash.h"
#include "ring.h"

unsigned char _last_state = STATE_SLEEPING;
bool _force_state = false;
bool _save_solve = true;
Ring _ring;
bool _scramble_update = false;

uint8_t g_user = 0;
uint8_t g_puzzle = PUZZLE_3x3x3;
unsigned char g_state = STATE_INTRO;
unsigned char g_mode = MODE_NONE;
s_settings g_settings;

bool scramble_update(uint8_t move) {

    if (_ring.available() == 0) return false;

    uint8_t reverse = cube_move_reverse(move);
    uint8_t sum = cube_move_sum(reverse, _ring.peek());
    int response;

    // cannot be summed (different face)
    if (sum == 0xFF) {
        response = _ring.prepend(reverse);
        utils_beep(BUZZER_FREQ_ERROR, BUZZER_DURATION * 3);
        if (response == -1) return false;
    
    // cancels the first movement
    } else if (sum == 0xFE) {
        response = _ring.read();
        if (response == -1) return false;

    // there is a non-null sum
    } else {
        response = _ring.replace(sum);
        if (response == -1) return false;

    }

    if (_ring.available() == 0) {
        g_state = STATE_INSPECT;
    } else {
        _scramble_update = true;
    }
    return true;

}



void touch_callback(unsigned char event) {
    
    if (event == TOUCH_EVENT_RELEASE) {
        
        TouchPointType point = touch_pointA();
        uint8_t button = display_get_button(point.x, point.y);
        #if DEBUG > 1
            Serial.printf("[MAIN] Button 0x%02X\n", button);
        #endif

        if (g_state == STATE_USER) {
            if (0xFF != button) {
                g_user = button;
                _force_state = true;
            }
        }

        if (g_state == STATE_USER_CONFIRM_RESET) {
            if (0 == button) reset_puzzle_user(g_puzzle, g_user);
            if (0xFF != button) g_state = STATE_USER;
        }

        if (g_state == STATE_SOLVED) {
            if (0 == button) {
                _save_solve = false;
                g_state = STATE_USER;
            }
        }

        if (g_state == STATE_TIMER) {
            if (g_mode == MODE_MANUAL) {
                cube_metrics_end(millis());
                utils_beep();
                g_state = STATE_SOLVED;
            }
        }

        if (g_state == STATE_INSPECT) {
            cube_metrics_start(millis());
            utils_beep();
            g_state = STATE_TIMER;
        }

        if (g_state == STATE_SCRAMBLE_MANUAL) {
            if (0 == button) g_state = STATE_INSPECT;
        }

        if (g_state == STATE_PUZZLES) {
            if (button < 6) {
                g_puzzle = button;
                g_state = STATE_USER;
            }
        }

        if (g_state == STATE_CONFIG) {
            switch (button) {

                case 0:
                    if (bluetooth_connected()) {
                        g_mode = MODE_SMARTCUBE;
                        g_state = STATE_USER;    
                    } else {
                        bluetooth_scan(true);
                        g_state = STATE_SMARTCUBE_CONNECT;
                    }
                    break;

                case 1:
                    if (stackmat_state() != STACKMAT_STATE_DISCONNECTED) {
                        if (g_mode == MODE_SMARTCUBE) bluetooth_disconnect();
                        g_mode = MODE_STACKMAT;
                        g_state = STATE_PUZZLES; 
                    } else {
                        g_state = STATE_STACKMAT_CONNECT;
                    }
                    break;

                case 2:
                    if (g_mode == MODE_SMARTCUBE) bluetooth_disconnect();
                    g_mode = MODE_MANUAL;
                    g_state = STATE_PUZZLES;
                    break;

                case 3:
                    g_state = STATE_SLEEPING;
                    break;

                default:
                    break;

            }
        }
            
        if (g_state == STATE_INTRO) {
            g_state = STATE_CONFIG;
        }

    }

    if (event == TOUCH_EVENT_LONG_CLICK) {
        if (g_state == STATE_USER) g_state = STATE_USER_CONFIRM_RESET;
        if (g_state == STATE_2D) cube_reset();
        if (g_state == STATE_3D) cube_reset();
    }

    if (event == TOUCH_EVENT_SWIPE_DOWN) {
        if (g_state == STATE_SCRAMBLE) g_state = STATE_USER;
        if (g_state == STATE_SCRAMBLE_MANUAL) g_state = STATE_USER;
        if (g_state == STATE_INSPECT) g_state = STATE_USER;
        if (g_state == STATE_TIMER) g_state = STATE_USER;
    }

    if (event == TOUCH_EVENT_SWIPE_LEFT) {

        if (g_mode == MODE_SMARTCUBE) {
            if (g_state == STATE_3D) g_state = STATE_SCRAMBLE;
            if (g_state == STATE_2D) {
                if (g_puzzle == PUZZLE_3x3x3) {
                    g_state = STATE_3D;
                } else {
                    g_state = STATE_INSPECT;
                }
            }
            if (g_state == STATE_USER) g_state = STATE_2D;
            if (g_state == STATE_CONFIG) g_state = STATE_USER;
        } else {
            if (g_state == STATE_USER) g_state = STATE_SCRAMBLE_MANUAL;
            if (g_state == STATE_CONFIG) g_state = STATE_PUZZLES;
        }
        if (g_state == STATE_PUZZLES) g_state = STATE_USER;
        if (g_state == STATE_SOLVED) g_state = STATE_USER;

    }

    if (event == TOUCH_EVENT_SWIPE_RIGHT) {

        if (g_state == STATE_PUZZLES) g_state = STATE_CONFIG;
        if (g_mode == MODE_SMARTCUBE) {
            if (g_state == STATE_USER) g_state = STATE_CONFIG;
            if (g_state == STATE_2D) g_state = STATE_USER;
            if (g_state == STATE_3D) g_state = STATE_2D;
            if ((g_puzzle == PUZZLE_3x3x3) && (g_state == STATE_SCRAMBLE)) g_state = STATE_3D;
            if ((g_puzzle != PUZZLE_3x3x3) && (g_state == STATE_SCRAMBLE)) g_state = STATE_USER;
        } else {
            if (g_state == STATE_USER) g_state = STATE_PUZZLES;
            if (g_state == STATE_SCRAMBLE_MANUAL) g_state = STATE_USER;
        }
        if (g_state == STATE_INSPECT) g_state = STATE_USER;
        if (g_state == STATE_SOLVED) g_state = STATE_USER;

    }

}

void stackmat_callback(unsigned char event, uint8_t * data) {

    #if DEBUG > 1
        Serial.printf("[MAIN] Stackmat event #%d\n", event);
    #endif

    if (event == STACKMAT_EVENT_DISCONNECT) {
        g_mode = MODE_NONE;
        g_state = STATE_CONFIG;
    }
    
    if (event == STACKMAT_EVENT_START) {
        if ((g_state == STATE_SCRAMBLE_MANUAL) || (g_state == STATE_INSPECT)) {
            cube_metrics_start(millis());
            utils_beep();
            g_state = STATE_TIMER;
        }
    }

    if (event == STACKMAT_EVENT_STOP) {
        if (g_state == STATE_TIMER) {
            
            cube_metrics_end(millis());
            utils_beep();
            g_state = STATE_SOLVED;
            
            // Sync time with stackmat
            uint32_t ms = (data[0] << 24)  + (data[1] << 16) + (data[2] << 8) + data[3];
            cube_time(ms);
            
        }
    }

    if (event == STACKMAT_EVENT_RESET) {
        g_state = STATE_SCRAMBLE_MANUAL;
    }


}

void cube_callback(unsigned char event, uint8_t * data) {

    switch (event) {

        case CUBE_EVENT_CONNECTED:
            utils_beep();
            g_state = STATE_USER;
            g_mode = MODE_SMARTCUBE;
            break;

        case CUBE_EVENT_DISCONNECTED:
            utils_beep();
            // TODO, check current state to avoid reset it
            g_mode = MODE_NONE;
            g_state = STATE_CONFIG;
            break;

        case CUBE_EVENT_4UTURNS:
            if (g_puzzle == PUZZLE_3x3x3) {
                if (g_state == STATE_SCRAMBLE) g_state = STATE_INSPECT;
                if ((g_state == STATE_USER) || (g_state == STATE_2D) || (g_state == STATE_3D)) g_state = STATE_SCRAMBLE;
            } else {
                g_state = STATE_INSPECT;
            }
            break;

        case CUBE_EVENT_MOVE:
            if (g_state == STATE_SOLVED) g_state = STATE_USER;
            if (g_state == STATE_INSPECT) {
                cube_metrics_start();
                utils_beep();
                g_state = STATE_TIMER;
            }
            if (g_state == STATE_SCRAMBLE) {
                if (!scramble_update(data[0])) {
                    g_state = STATE_2D;    
                }
            }
            break;

        case CUBE_EVENT_SOLVED:
            if (g_state == STATE_TIMER) {
                cube_metrics_end();
                utils_beep();
                g_state = STATE_SOLVED;
            }
            break;

    }

}

void reset_puzzle_user(uint8_t puzzle, uint8_t user) {

    g_settings.puzzle[puzzle].user[user].best.time = 0;
    g_settings.puzzle[puzzle].user[user].best.turns = 0;
    g_settings.puzzle[puzzle].user[user].best.tps = 0;

    g_settings.puzzle[puzzle].user[user].ao5.time = 0;
    g_settings.puzzle[puzzle].user[user].ao5.turns = 0;
    g_settings.puzzle[puzzle].user[user].ao5.tps = 0;
    
    g_settings.puzzle[puzzle].user[user].ao12.time = 0;
    g_settings.puzzle[puzzle].user[user].ao12.turns = 0;
    g_settings.puzzle[puzzle].user[user].ao12.tps = 0;
    
    for (uint8_t i=0; i<12; i++) {
        g_settings.puzzle[puzzle].user[user].solve[i].time = 0;
        g_settings.puzzle[puzzle].user[user].solve[i].turns = 0;
        g_settings.puzzle[puzzle].user[user].solve[i].tps = 0;
    }

    flash_save();

}

void add_solve(uint8_t puzzle, uint8_t user, uint32_t time, uint16_t turns) {

    bool has_avg5 = true;
    bool has_avg12 = true;
    uint32_t time_sum5 = time;
    uint32_t time_sum5_min = time;
    uint32_t time_sum5_max = time;
    uint32_t time_sum12 = time;
    uint32_t time_sum12_min = time;
    uint32_t time_sum12_max = time;
    uint32_t turns_sum5 = turns;
    uint32_t turns_sum12 = turns;
    uint16_t tps = 100 * utils_tps(time, turns);

    // Move solves
    for (int8_t i=10; i>=0; i--) {
        if (g_settings.puzzle[puzzle].user[user].solve[i].time > 0) {
            if (time_sum12_min > g_settings.puzzle[puzzle].user[user].solve[i].time) time_sum12_min = g_settings.puzzle[puzzle].user[user].solve[i].time;
            if (time_sum12_max < g_settings.puzzle[puzzle].user[user].solve[i].time) time_sum12_max = g_settings.puzzle[puzzle].user[user].solve[i].time;
            time_sum12 += g_settings.puzzle[puzzle].user[user].solve[i].time;
            turns_sum12 += g_settings.puzzle[puzzle].user[user].solve[i].turns;
            if (i<4) {
                if (time_sum5_min > g_settings.puzzle[puzzle].user[user].solve[i].time) time_sum5_min = g_settings.puzzle[puzzle].user[user].solve[i].time;
                if (time_sum5_max < g_settings.puzzle[puzzle].user[user].solve[i].time) time_sum5_max = g_settings.puzzle[puzzle].user[user].solve[i].time;
                time_sum5 += g_settings.puzzle[puzzle].user[user].solve[i].time;
                turns_sum5 += g_settings.puzzle[puzzle].user[user].solve[i].turns;
            }
        } else {
            has_avg12 = false;
            if (i<4) has_avg5 = false;
        }
        g_settings.puzzle[puzzle].user[user].solve[i+1].time = g_settings.puzzle[puzzle].user[user].solve[i].time;
        g_settings.puzzle[puzzle].user[user].solve[i+1].turns = g_settings.puzzle[puzzle].user[user].solve[i].turns;
        g_settings.puzzle[puzzle].user[user].solve[i+1].tps = g_settings.puzzle[puzzle].user[user].solve[i].tps;
    }

    // Save last solve
    g_settings.puzzle[puzzle].user[user].solve[0].time = time;
    g_settings.puzzle[puzzle].user[user].solve[0].turns = turns;
    g_settings.puzzle[puzzle].user[user].solve[0].tps = tps;

    // Save best solve
    {
        if (g_settings.puzzle[puzzle].user[user].best.time == 0) {
            g_settings.puzzle[puzzle].user[user].best.time = time;
            g_settings.puzzle[puzzle].user[user].best.tps = tps;
        } else {
            if (time < g_settings.puzzle[puzzle].user[user].best.time) g_settings.puzzle[puzzle].user[user].best.time = time;
            if (tps > g_settings.puzzle[puzzle].user[user].best.tps) g_settings.puzzle[puzzle].user[user].best.tps = tps;
        }
    }

    // Save ao5
    if (has_avg5) {
        g_settings.puzzle[puzzle].user[user].ao5.time = (time_sum5 - time_sum5_min - time_sum5_max) / 3;
        g_settings.puzzle[puzzle].user[user].ao5.turns = turns_sum5 / 5;
        g_settings.puzzle[puzzle].user[user].ao5.tps = 100 * utils_tps(time_sum5, turns_sum5);
    }

    // Save ao12
    if (has_avg12) {
        g_settings.puzzle[puzzle].user[user].ao12.time = (time_sum12 - time_sum12_min - time_sum12_max) / 10;
        g_settings.puzzle[puzzle].user[user].ao12.turns = turns_sum12 / 12;
        g_settings.puzzle[puzzle].user[user].ao12.tps = 100 * utils_tps(time_sum12, turns_sum12);
    }

}


void state_machine() {

    static unsigned long last_change = millis();
    bool changed_display = false;
    bool save_flash = false;
    bool changed_state = (_last_state != g_state) || _force_state;
    _force_state = false;
    
    // Handle state exit cases    
    if ((_last_state != g_state) && (_last_state == STATE_SOLVED)) {
        if (_save_solve) {
            unsigned long time = cube_time();
            unsigned short turns = cube_turns();
            if (time > 0) {
                add_solve(g_puzzle, g_user, time, turns);
                save_flash = true;
            }
        }
        _save_solve = true;
    }

    if (changed_state) {
        _last_state = g_state;
        last_change = millis();
        #if DEBUG>1
            Serial.printf("[MAIN] State %d, Mode %d\n", g_state, g_mode);
        #endif
    }

    display_start_transaction();

    switch (g_state) {

        case STATE_SLEEPING:
            utils_sleep();
            g_state = STATE_INTRO;
            break;

        case STATE_INTRO:
            if (changed_state) {
                display_page_intro();
                changed_display = true;
            }
            if (millis() - last_change > INTRO_TIMEOUT) {
                g_state = STATE_CONFIG;
            }
            break;

        case STATE_CONFIG:
            if (changed_state) {
                display_page_config(g_mode);
                changed_display = true;
            }
            if (millis() - last_change > SHUTDOWN_TIMEOUT) {
                g_state = STATE_SLEEPING;
            }
            break;

        case STATE_SMARTCUBE_CONNECT:
            if (changed_state) {
                display_page_smartcube_connect();
                changed_display = true;
            }
            if (millis() - last_change > CONNECT_TIMEOUT) {
                bluetooth_scan(false);
                g_state = STATE_CONFIG;
            }
            break;

        case STATE_STACKMAT_CONNECT:
            if (changed_state) {
                display_page_stackmat_connect();
                changed_display = true;
            }
            if (stackmat_state() != STACKMAT_STATE_DISCONNECTED) {
                if (g_mode == MODE_SMARTCUBE) bluetooth_disconnect();
                g_mode = MODE_STACKMAT;
                g_state = STATE_USER;
            }
            if (millis() - last_change > CONNECT_TIMEOUT) {
                g_state = STATE_CONFIG;
            }
            break;

        case STATE_PUZZLES:
            if (changed_state) {
                display_page_puzzles(g_puzzle);
                changed_display = true;
            }
            if (millis() - last_change > SHUTDOWN_TIMEOUT) {
                g_state = STATE_SLEEPING;
            }
            break;

        case STATE_USER:
            if (changed_state) {
                display_page_user(g_puzzle, g_user);
                changed_display = true;
            }
            break;

        case STATE_2D:
            if (changed_state || cube_updated()) {
                display_page_2d();
                changed_display = true;
            }
            break;

        case STATE_3D:
            if (changed_state || cube_updated()) {
                display_page_3d();
                changed_display = true;
            }
            break;

        case STATE_USER_CONFIRM_RESET:
            if (changed_state) {
                display_page_user_confirm_reset(g_user);
                changed_display = true;
            }
            break;

        case STATE_SCRAMBLE:
            if (changed_state) {
                cube_scramble(&_ring, SCRAMBLE_SIZE);
                _scramble_update = true;
            }
            if (_scramble_update) {
                display_page_scramble(&_ring);
                changed_display = true;
                _scramble_update = false;
            }
            break;

        case STATE_SCRAMBLE_MANUAL:
            if (changed_state) {
                cube_scramble(&_ring, SCRAMBLE_SIZE);
                _scramble_update = true;
            }
            if (_scramble_update) {
                display_page_scramble_manual(&_ring);
                changed_display = true;
                _scramble_update = false;
            }
            break;

        case STATE_INSPECT:
            if (changed_state) {
                display_page_inspect();
                changed_display = true;
            }
            break;

        case STATE_TIMER:
            display_page_timer();
            changed_display = true;
            break;

        case STATE_SOLVED:
            if (changed_state) {
                unsigned long time = cube_time();
                if (time == 0) {
                    g_state = STATE_2D;
                } else {
                    display_page_solved();
                    changed_display = true;
                }
            }
            break;
        
        default:
            break;

    }

    if (changed_display) {
        display_end_transaction();
    }
    if (save_flash) {
        flash_save();
    }

}

void setup() {
    
    #ifdef DEBUG
        Serial.begin(115200);
		while ((!Serial) && (millis() < 2000)) delay(10);
		Serial.printf("\n[MAIN] %s %s\n\n", APP_NAME, APP_VERSION);
    #endif

    wdt_set(WDT_SECONDS);
    
    utils_setup();
    flash_setup();
    flash_load();
    bluetooth_setup();
    display_setup();
    cube_setup();
    stackmat_setup();

    while (!touch_setup(TOUCH_INT_PIN)) {
	    Serial.println("[MAIN] Touch interface is not connected");
		delay(1000);
    }

    // Callback to be called upon event
    touch_set_callback(touch_callback);
    cube_set_callback(cube_callback);
    stackmat_set_callback(stackmat_callback);

}

void loop() {

    //bluetooth_loop();
    stackmat_loop();
    touch_loop();
    //display_loop();
    state_machine();
    wdt_feed();

    utils_delay(1);

}

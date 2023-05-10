#include <Arduino.h>
#include <SPI.h>

#include "config.h"
#include "bluetooth.h"
#include "utils.h"
#include "display.h"
#include "touch.h"
#include "cube.h"
#include "flash.h"
#include "ring.h"

enum {
    STATE_SLEEPING,
    STATE_INTRO,
    STATE_USER,
    STATE_USER_CONFIRM_RESET,
    STATE_2D,
    STATE_3D,
    STATE_SCRAMBLE,
    STATE_INSPECT,
    STATE_TIMER,
    STATE_SOLVED,
    STATE_DISCONNECT
};

unsigned char _last_state = STATE_SLEEPING;
unsigned char _state = STATE_INTRO;
bool _force_state = false;
Ring _ring;
bool _scramble_update = false;

uint8_t g_user = 0;
s_settings g_settings;

bool scramble_update(uint8_t move) {

    if (_ring.available() == 0) return false;

    uint8_t reverse = cube_move_reverse(move);
    uint8_t sum = cube_move_sum(reverse, _ring.peek());
    int response;

    // cannot be summed (different face)
    if (sum == 0xFF) {
        response = _ring.prepend(reverse);
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
        _state = STATE_INSPECT;
    } else {
        _scramble_update = true;
    }
    return true;

}



void touch_callback(unsigned char event) {
    
    if (event == TOUCH_EVENT_RELEASE) {
        
        if (_state == STATE_USER) {
            TouchPointType point = touch_pointA();
            if (point.y < 60) {
                g_user = point.x / 60;
                _force_state = true;
            }
        }

        if (_state == STATE_USER_CONFIRM_RESET) {
            TouchPointType point = touch_pointA();
            if ((120 < point.x) & (point.x < 160)) {
                if (point.y > 160) {
                    reset_user(g_user);
                }
            }
            _state = STATE_USER;
        }

    }

    if (event == TOUCH_EVENT_LONG_CLICK) {
        if (_state == STATE_USER) _state = STATE_USER_CONFIRM_RESET;
    }

    if (event == TOUCH_EVENT_SWIPE_DOWN) {

        switch (_state) {

            case STATE_INTRO:
                _state = STATE_SLEEPING;
                break;

            case STATE_USER:
            case STATE_2D:
            case STATE_3D:
                _state = STATE_DISCONNECT;
                break;

            case STATE_INSPECT:
            case STATE_TIMER:
            case STATE_SOLVED:
            case STATE_SCRAMBLE:
                _state = STATE_2D;
                break;

            default:
                break;

        };

    }

    if (event == TOUCH_EVENT_SWIPE_LEFT) {

        if (_state == STATE_3D) _state = STATE_SCRAMBLE;
        if (_state == STATE_2D) _state = STATE_3D;
        if (_state == STATE_USER) _state = STATE_2D;

    }

    if (event == TOUCH_EVENT_SWIPE_RIGHT) {

        if (_state == STATE_2D) _state = STATE_USER;
        if (_state == STATE_3D) _state = STATE_2D;
        if (_state == STATE_SCRAMBLE) _state = STATE_3D;

    }

}

void cube_callback(unsigned char event, uint8_t * data) {

    switch (event) {

        case CUBE_EVENT_CONNECTED:
            utils_beep();
            _state = STATE_USER;
            break;

        case CUBE_EVENT_DISCONNECTED:
            utils_beep();
            _state = STATE_INTRO;
            break;

        case CUBE_EVENT_4UTURNS:
            if ((_state == STATE_2D) || (_state == STATE_3D)) _state = STATE_INSPECT;
            break;

        case CUBE_EVENT_MOVE:
            //if (_state == STATE_USER) _state = STATE_2D;
            if (_state == STATE_SOLVED) _state = STATE_2D;
            if (_state == STATE_INSPECT) {
                cube_metrics_start();
                utils_beep();
                _state = STATE_TIMER;
            }
            if (_state == STATE_SCRAMBLE) {
                if (!scramble_update(data[0])) {
                    _state = STATE_2D;    
                }
            }
            break;

        case CUBE_EVENT_SOLVED:
            if (_state == STATE_TIMER) {
                cube_metrics_end();
                utils_beep();
                _state = STATE_SOLVED;
            }
            break;

    }

}

void reset_user(uint8_t user) {

    g_settings.user[user].best.time = 0;
    g_settings.user[user].best.turns = 0;
    g_settings.user[user].best.tps = 0;

    g_settings.user[user].av5.time = 0;
    g_settings.user[user].av5.turns = 0;
    g_settings.user[user].av5.tps = 0;
    
    g_settings.user[user].av12.time = 0;
    g_settings.user[user].av12.turns = 0;
    g_settings.user[user].av12.tps = 0;
    
    for (uint8_t i=0; i<12; i++) {
        g_settings.user[user].solve[i].time = 0;
        g_settings.user[user].solve[i].turns = 0;
        g_settings.user[user].solve[i].tps = 0;
    }

    flash_save();

}

void add_solve(uint8_t user, uint32_t time, uint16_t turns) {

    bool has_avg5 = true;
    bool has_avg12 = true;
    uint32_t time_sum5 = time;
    uint32_t time_sum12 = time;
    uint32_t turns_sum5 = turns;
    uint32_t turns_sum12 = turns;
    uint16_t tps = 100 * utils_tps(time, turns);

    // Move solves
    for (int8_t i=10; i>=0; i--) {
        if (g_settings.user[user].solve[i].time > 0) {
            time_sum12 += g_settings.user[user].solve[i].time;
            turns_sum12 += g_settings.user[user].solve[i].turns;
            if (i<4) {
                time_sum5 += g_settings.user[user].solve[i].time;
                turns_sum5 += g_settings.user[user].solve[i].turns;
            }
        } else {
            has_avg12 = false;
            if (i<4) has_avg5 = false;
        }
        g_settings.user[user].solve[i+1].time = g_settings.user[user].solve[i].time;
        g_settings.user[user].solve[i+1].turns = g_settings.user[user].solve[i].turns;
        g_settings.user[user].solve[i+1].tps = g_settings.user[user].solve[i].tps;
    }

    // Save last solve
    g_settings.user[user].solve[0].time = time;
    g_settings.user[user].solve[0].turns = turns;
    g_settings.user[user].solve[0].tps = tps;

    // Save best solve
    {
        if (g_settings.user[user].best.time == 0) {
            g_settings.user[user].best.time = time;
            g_settings.user[user].best.tps = tps;
        } else {
            if (time < g_settings.user[user].best.time) g_settings.user[user].best.time = time;
            if (tps > g_settings.user[user].best.tps) g_settings.user[user].best.tps = tps;
        }
    }

    // Save av5
    if (has_avg5) {
        g_settings.user[user].av5.time = time_sum5 / 5;
        g_settings.user[user].av5.turns = turns_sum5 / 5;
        g_settings.user[user].av5.tps = 100 * utils_tps(time_sum5, turns_sum5);
    }

    // Save av12
    if (has_avg12) {
        g_settings.user[user].av12.time = time_sum12 / 12;
        g_settings.user[user].av12.turns = turns_sum12 / 12;
        g_settings.user[user].av12.tps = 100 * utils_tps(time_sum12, turns_sum12);
    }

}


void state_machine() {

    static unsigned long last_change = millis();
    bool changed_display = false;
    bool save_flash = false;
    bool changed_state = (_last_state != _state) || _force_state;
    _force_state = false;
    _last_state = _state;

    if (changed_state) {
        last_change = millis();
        #if DEBUG>1
            Serial.printf("[MAIN] State %d\n", _state);
        #endif
    }

    display_start_transaction();

    switch (_state) {

        case STATE_SLEEPING:
            bluetooth_disconnect();
            delay(10);
            sleep();
            _state = STATE_INTRO;
            break;

        case STATE_INTRO:
            if (changed_state) {
                display_page_intro();
                changed_display = true;
            }
            if (millis() - last_change > SHUTDOWN_TIMEOUT) {
                _state = STATE_SLEEPING;
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

        case STATE_USER:
            if (changed_state) {
                display_page_user(g_user);
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
                unsigned short turns = cube_turns();
                if (time == 0) {
                    _state = STATE_2D;
                } else {

                    add_solve(g_user, time, turns);
                    save_flash = true;

                    display_page_solved();
                    changed_display = true;
                }

            }
            break;
        
        case STATE_DISCONNECT:
            bluetooth_disconnect();
            _state = STATE_INTRO;
        
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
    bluetooth_scan(true);
    display_setup();
    cube_setup();
    cube_set_callback(cube_callback);

    while (!touch_setup(TOUCH_INT_PIN)) {
	    Serial.println("[MAIN] Touch interface is not connected");
		delay(1000);
    }

    // Callback to be called upn event
    touch_set_callback(touch_callback);

}

void loop() {

    static unsigned long last = 0;
    if (millis() - last < 10) return;
    last = millis();

    bluetooth_loop();
    touch_loop();
    display_loop();
    state_machine();
    wdt_feed();

}

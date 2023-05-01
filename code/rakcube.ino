#include <Arduino.h>
#include <SPI.h>

#include "config.h"
#include "bluetooth.h"
#include "utils.h"
#include "display.h"
#include "touch.h"
#include "cube.h"

enum {
    STATE_SLEEPING,
    STATE_INTRO,
    STATE_CONNECTED,
    STATE_INSPECT,
    STATE_TIMER,
    STATE_RESULT,
    STATE_DISCONNECT
};

unsigned char _state_old = STATE_SLEEPING;
unsigned char _state = STATE_INTRO;

void touch_callback(unsigned char event) {
    
    if (event == TOUCH_EVENT_SWIPE_DOWN) {

        switch (_state) {

            case STATE_INTRO:
                _state = STATE_SLEEPING;
                break;

            case STATE_CONNECTED:
                _state = STATE_DISCONNECT;
                break;

            case STATE_INSPECT:
            case STATE_TIMER:
            case STATE_RESULT:
                _state = STATE_CONNECTED;
                break;

            default:
                break;

        };

    }

}

void cube_callback(unsigned char event) {

    switch (event) {

        case CUBE_EVENT_CONNECTED:
            _state = STATE_CONNECTED;
            break;

        case CUBE_EVENT_DISCONNECTED:
            _state = STATE_INTRO;
            break;

        case CUBE_EVENT_4UTURNS:
            if (_state == STATE_CONNECTED) _state = STATE_INSPECT;
            break;

        case CUBE_EVENT_MOVE:
            if (_state == STATE_INSPECT) _state = STATE_TIMER;
            if (_state == STATE_RESULT) _state = STATE_CONNECTED;
            break;

        case CUBE_EVENT_SOLVED:
            if (_state == STATE_TIMER) _state = STATE_RESULT;
            break;

    }

}

void state_machine() {

    static unsigned long last_change = millis();
    bool changed_display = false;
    bool changed_state = (_state_old != _state);
    _state_old = _state;

    if (changed_state) {
        last_change = millis();
        #if DEBUG>1
            Serial.printf("[MAIN] State %d\n", _state);
        #endif
    }

    display_start_transaction();

    switch (_state) {

        case STATE_SLEEPING:
            sleep();
            _state = STATE_INTRO;
            break;

        case STATE_INTRO:
            if (changed_state) {
                display_show_intro();
                changed_display = true;
            }
            if (millis() - last_change > SHUTDOWN_TIMEOUT) {
                _state = STATE_SLEEPING;
            }
            break;

        case STATE_CONNECTED:
            if (changed_state) {
                cube_reset();
            }
            if (cube_updated()) {
                display_update_cube();
                display_battery();
                changed_display = true;
            }
            break;

        case STATE_INSPECT:
            if (changed_state) {
                display_update_cube();
                display_battery();
                display_show_ready();
                changed_display = true;
            }
            break;

        case STATE_TIMER:
            display_update_cube();
            display_battery();
            display_show_timer();
            changed_display = true;
            break;

        case STATE_RESULT:
            if (changed_state) {
                display_update_cube();
                display_battery();
                display_show_timer();
                changed_display = true;
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

}

void setup() {
    
    #ifdef DEBUG
        Serial.begin(115200);
		while ((!Serial) && (millis() < 2000)) delay(10);
		Serial.printf("\n[MAIN] RAKcube\n\n");
    #endif

    wdt_set(WDT_SECONDS);
    
    utils_setup();
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

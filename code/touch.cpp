#include <Arduino.h>
#include "RAK14014_FT6336U.h"

#include "touch.h"

void (*_touch_callback)(unsigned char);

TouchPointType _touch_pointA;
TouchPointType _touch_pointB;
unsigned char _touch_state = 0;
unsigned char _touch_event = 0;
unsigned long _touch_event_start = 0;

static unsigned char _touch_flag = false;

FT6336U _touch_interface; 

// ----------------------------------------------------------------------------
// Private
// ----------------------------------------------------------------------------

void touch_send_event(unsigned char event) {
    if (event != _touch_event) {
        #if DEBUG > 1
            Serial.printf("[TCH] Touch event %d\n", event);
        #endif
        _touch_event = event;
        if (_touch_callback) _touch_callback(_touch_event);
    }
}

void touch_process(void) {
    
    // get new status
    uint8_t newstate = _touch_interface.read_td_status();

    // Same state, check for time
    if (newstate == _touch_state) {
        if (_touch_state != 0) {
            if ((millis() - _touch_event_start) > 1000) {
                touch_send_event(FT6336U_EVENT_HOLD);
            }
        }
    
    // Different event
    } else {

        // click
        if ((_touch_state == 0) && (newstate == 1)) {
            touch_send_event(FT6336U_EVENT_CLICK);
            _touch_event_start = millis();
            _touch_pointA.x = _touch_interface.read_touch1_x();
            _touch_pointA.y = _touch_interface.read_touch1_y();
        }

        // release
        if ((_touch_state == 1) && (newstate == 0)) {
            touch_send_event(FT6336U_EVENT_RELEASE);
            _touch_pointB.x = _touch_interface.read_touch1_x();
            _touch_pointB.y = _touch_interface.read_touch1_y();
            if (_touch_pointB.x - _touch_pointA.x > 80) {
                touch_send_event(FT6336U_EVENT_SWIPE_UP);
            } else if (_touch_pointA.x - _touch_pointB.x > 80) {
                touch_send_event(FT6336U_EVENT_SWIPE_DOWN);
            } else if (_touch_pointB.y - _touch_pointA.y > 60) {
                touch_send_event(FT6336U_EVENT_SWIPE_RIGHT);
            } else if (_touch_pointA.y - _touch_pointB.y > 60) {
                touch_send_event(FT6336U_EVENT_SWIPE_LEFT);
            } else {
                touch_send_event(FT6336U_EVENT_RELEASE);
            }
        }

        // click two fingers
        if ((_touch_state == 0) && (newstate == 2)) {
            touch_send_event(FT6336U_EVENT_CLICK_TWO_FINGERS);
            _touch_event_start = millis();
            _touch_pointA.x = _touch_interface.read_touch1_x();
            _touch_pointA.y = _touch_interface.read_touch1_y();
            _touch_pointB.x = _touch_interface.read_touch2_x();
            _touch_pointB.y = _touch_interface.read_touch2_y();
        }

        // release two fingers
        if ((_touch_state == 2) && (newstate == 0)) {
            touch_send_event(FT6336U_EVENT_RELEASE);
        }

        _touch_state = newstate;

    }

}

// ----------------------------------------------------------------------------
// Public
// ----------------------------------------------------------------------------

unsigned char touch_event() {
    return _touch_event;
}

TouchPointType touch_pointA() {
    return _touch_pointA;
}

TouchPointType touch_pointB() {
    return _touch_pointB;
}

void touch_set_callback(void (*callback)(unsigned char type)) {
    _touch_callback = callback;
}

bool touch_setup(unsigned char interrupt_pin) {
    
    if (!_touch_interface.begin()) return false;

    pinMode(interrupt_pin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(interrupt_pin), [](void) { _touch_flag = true; }, FALLING);

    #if DEBUG > 0
        Serial.print("[TCH] FT6336U Firmware Version: "); 
        Serial.println(_touch_interface.read_firmware_id());  
        Serial.print("[TCH] FT6336U Device Mode: "); 
        Serial.println(_touch_interface.read_device_mode());  
    #endif

    return true;

}

void touch_loop() {
    if (_touch_flag) {
        _touch_flag = false;
        touch_process();
    }
}

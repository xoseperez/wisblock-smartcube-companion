#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <SoftwareSerial.h>

#include "timer.h"
#include "config.h"

#ifdef TIMER_SERIAL
    #define _timer_serial TIMER_SERIAL
#else
    SoftwareSerial _timer_serial(TIMER_RX_PIN, TIMER_TX_PIN, true);
#endif

void (*_timer_callback)(uint8_t event, uint8_t * data);
unsigned char _timer_state = TIMER_STATE_DISCONNECTED;
unsigned long _timer_last = 0;
char _timer_buffer[10] = {0};
unsigned char _timer_pointer = 0;

void timer_set_callback(void (*callback)(uint8_t event, uint8_t * data)) {
    _timer_callback = callback;
}

uint8_t timer_state() {
    return _timer_state;
}

void timer_process() {

    _timer_last = millis();
    
    if (_timer_state == TIMER_STATE_DISCONNECTED) {
        if (strncmp(_timer_buffer, "I000000", 7) == 0) {
            _timer_state = TIMER_STATE_STOPPED;
        } else {
            _timer_state = TIMER_STATE_WAITING;
        }
        if (_timer_callback) _timer_callback(TIMER_EVENT_CONNECT, nullptr);
    }

    if (_timer_state == TIMER_STATE_WAITING) {
        if (_timer_buffer[0] == ' ') {
            _timer_state = TIMER_STATE_RUNNING;
            if (_timer_callback) _timer_callback(TIMER_EVENT_START, nullptr);
        }
    }

    if (_timer_state == TIMER_STATE_RUNNING) {
        
        if (_timer_buffer[0] == 'I') {

            unsigned long ms = 0;
            ms += ((_timer_buffer[6] - '0') *     1);
            ms += ((_timer_buffer[5] - '0') *    10);
            ms += ((_timer_buffer[4] - '0') *   100);
            ms += ((_timer_buffer[3] - '0') *  1000);
            ms += ((_timer_buffer[2] - '0') * 10000);
            ms += ((_timer_buffer[1] - '0') * 60000);

            uint8_t data[4] = {0};
            data[0] = (ms >> 24) & 0xFF;
            data[1] = (ms >> 16) & 0xFF;
            data[2] = (ms >>  8) & 0xFF;
            data[3] = (ms >>  0) & 0xFF;

            _timer_state = TIMER_STATE_STOPPED;
            if (_timer_callback) _timer_callback(TIMER_EVENT_STOP, data);
            
        }

    }

    if (_timer_state == TIMER_STATE_STOPPED) {
        if (strncmp(_timer_buffer, "I000000", 7) == 0) {
            _timer_state = TIMER_STATE_WAITING;
            if (_timer_callback) _timer_callback(TIMER_EVENT_RESET, nullptr);
        }
    }

}

void timer_setup() {
    _timer_serial.begin(TIMER_BAUDRATE);
}

void timer_loop() {
    
    while (_timer_serial.available()) {
        unsigned char c = _timer_serial.read();
        if (c == 13) {
            _timer_buffer[_timer_pointer++] = 0;
            timer_process();
            _timer_pointer = 0;
        } else if (_timer_pointer < sizeof(_timer_buffer) - 1 ) {
            _timer_buffer[_timer_pointer] = c;
            _timer_pointer++;
        }
    }

    if (_timer_state != TIMER_STATE_DISCONNECTED) {
        if (millis() - _timer_last > TIMER_TIMEOUT) {
            _timer_state = TIMER_STATE_DISCONNECTED;
            if (_timer_callback) _timer_callback(TIMER_EVENT_DISCONNECT, nullptr);
        }
    }

}
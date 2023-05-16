#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <SoftwareSerial.h>

#include "stackmat.h"
#include "config.h"

SoftwareSerial _stackmat_serial(STACKMAT_RX_PIN, STACKMAT_TX_PIN, true);
void (*_stackmat_callback)(uint8_t event, uint8_t * data);
unsigned char _stackmat_state = STACKMAT_STATE_DISCONNECTED;
unsigned long _stackmat_last = 0;
char _stackmat_buffer[10] = {0};
unsigned char _stackmat_pointer = 0;

void stackmat_set_callback(void (*callback)(uint8_t event, uint8_t * data)) {
    _stackmat_callback = callback;
}

uint8_t stackmat_state() {
    return _stackmat_state;
}

void stackmat_process() {

    _stackmat_last = millis();
    
    if (_stackmat_state == STACKMAT_STATE_DISCONNECTED) {
        if (strncmp(_stackmat_buffer, "I000000", 7) == 0) {
            _stackmat_state = STACKMAT_STATE_STOPPED;
        } else {
            _stackmat_state = STACKMAT_STATE_WAITING;
        }
        if (_stackmat_callback) _stackmat_callback(STACKMAT_EVENT_CONNECT, nullptr);
    }

    if (_stackmat_state == STACKMAT_STATE_WAITING) {
        if (_stackmat_buffer[0] == ' ') {
            _stackmat_state = STACKMAT_STATE_RUNNING;
            if (_stackmat_callback) _stackmat_callback(STACKMAT_EVENT_START, nullptr);
        }
    }

    if (_stackmat_state == STACKMAT_STATE_RUNNING) {
        
        unsigned long ms = 0;
        ms += ((_stackmat_buffer[6] - '0') *     1);
        ms += ((_stackmat_buffer[5] - '0') *    10);
        ms += ((_stackmat_buffer[4] - '0') *   100);
        ms += ((_stackmat_buffer[3] - '0') *  1000);
        ms += ((_stackmat_buffer[2] - '0') * 10000);
        ms += ((_stackmat_buffer[1] - '0') * 60000);

        uint8_t data[4] = {0};
        data[0] = (ms >> 24) & 0xFF;
        data[1] = (ms >> 16) & 0xFF;
        data[2] = (ms >>  8) & 0xFF;
        data[3] = (ms >>  0) & 0xFF;

        if (_stackmat_buffer[0] == 'I') {
            _stackmat_state = STACKMAT_STATE_STOPPED;
            if (_stackmat_callback) _stackmat_callback(STACKMAT_EVENT_STOP, data);
        }

    }

    if (_stackmat_state == STACKMAT_STATE_STOPPED) {
        if (strncmp(_stackmat_buffer, "I000000", 7) == 0) {
            _stackmat_state = STACKMAT_STATE_WAITING;
            if (_stackmat_callback) _stackmat_callback(STACKMAT_EVENT_RESET, nullptr);
        }
    }

}

void stackmat_setup() {
    _stackmat_serial.begin(STACKMAT_BAUDRATE);
}

void stackmat_loop() {
    
    while (_stackmat_serial.available()) {
        unsigned char c = _stackmat_serial.read();
        if (c == 13) {
            _stackmat_buffer[_stackmat_pointer++] = 0;
            stackmat_process();
            _stackmat_pointer = 0;
        } else if (_stackmat_pointer < sizeof(_stackmat_buffer) - 1 ) {
            _stackmat_buffer[_stackmat_pointer] = c;
            _stackmat_pointer++;
        }
    }

    if (_stackmat_state != STACKMAT_STATE_DISCONNECTED) {
        if (millis() - _stackmat_last > STACKMAT_TIMEOUT) {
            _stackmat_state = STACKMAT_STATE_DISCONNECTED;
            if (_stackmat_callback) _stackmat_callback(STACKMAT_EVENT_DISCONNECT, nullptr);
        }
    }

    delay(1);

}
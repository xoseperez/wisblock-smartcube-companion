#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <SoftwareSerial.h>

#include "timer.h"
#include "config.h"

#ifdef TIMER_SERIAL
    #define _stackmat_serial TIMER_SERIAL
#else
    SoftwareSerial _stackmat_serial(TIMER_RX_PIN, TIMER_TX_PIN, true);
#endif

void (*_stackmat_callback)(uint8_t event, uint8_t * data);
unsigned char _stackmat_state = TIMER_STATE_DISCONNECTED;
unsigned long _stackmat_last = 0;
char _stackmat_buffer[10] = {0};
unsigned char _stackmat_pointer = 0;

TaskHandle_t _stackmat_task_handle;
SemaphoreHandle_t _stackmat_task_semaphore = NULL;

// ----------------------------------------------------------------------------

void stackmat_process() {

    _stackmat_last = millis();
    
    if (_stackmat_state == TIMER_STATE_DISCONNECTED) {
        if (strncmp(_stackmat_buffer, "I000000", 7) == 0) {
            _stackmat_state = TIMER_STATE_STOPPED;
        } else {
            _stackmat_state = TIMER_STATE_WAITING;
        }
        if (_stackmat_callback) _stackmat_callback(TIMER_EVENT_CONNECT, nullptr);
    }

    if (_stackmat_state == TIMER_STATE_WAITING) {
        if (_stackmat_buffer[0] == ' ') {
            _stackmat_state = TIMER_STATE_RUNNING;
            if (_stackmat_callback) _stackmat_callback(TIMER_EVENT_START, nullptr);
        }
    }

    if (_stackmat_state == TIMER_STATE_RUNNING) {
        
        if (_stackmat_buffer[0] == 'I') {

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

            _stackmat_state = TIMER_STATE_STOPPED;
            if (_stackmat_callback) _stackmat_callback(TIMER_EVENT_STOP, data);
            
        }

    }

    if (_stackmat_state == TIMER_STATE_STOPPED) {
        if (strncmp(_stackmat_buffer, "I000000", 7) == 0) {
            _stackmat_state = TIMER_STATE_WAITING;
            if (_stackmat_callback) _stackmat_callback(TIMER_EVENT_RESET, nullptr);
        }
    }

}

void stackmat_loop() {
    
    if (xSemaphoreTake(_stackmat_task_semaphore, portMAX_DELAY) == pdTRUE) {
    
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

        if (_stackmat_state != TIMER_STATE_DISCONNECTED) {
            if (millis() - _stackmat_last > TIMER_TIMEOUT) {
                _stackmat_state = TIMER_STATE_DISCONNECTED;
                if (_stackmat_callback) _stackmat_callback(TIMER_EVENT_DISCONNECT, nullptr);
            }
        }
    
    }

}

void stackmat_task(void *pvParameters) {
	while(true) stackmat_loop();
}

void stackmat_set_callback(void (*callback)(uint8_t event, uint8_t * data)) {
    _stackmat_callback = callback;
}

uint8_t stackmat_state() {
    return _stackmat_state;
}

void stackmat_enable(bool enable) {
    if (enable) {
        _stackmat_serial.flush();
        xSemaphoreGive(_stackmat_task_semaphore);    
    } else {
        xSemaphoreTake(_stackmat_task_semaphore, 10);
    }
}

bool stackmat_init() {

    // Initialize serial
    _stackmat_serial.begin(TIMER_BAUDRATE);

    // Create semaphore for local task
    _stackmat_task_semaphore = xSemaphoreCreateBinary();
    stackmat_enable(false);
	
    // Create local task
	if (!xTaskCreate(stackmat_task, "STACKMAT", 8192, NULL, TASK_PRIO_NORMAL, &_stackmat_task_handle)) {
		return false;
	}

    return true;

}


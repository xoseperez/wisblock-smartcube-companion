#include <Arduino.h>
#include <bluefruit.h>

#include "config.h"
#include "bluetooth.h"
#include "timer.h"
#include "gantimer.h"

void (*_gantimer_callback)(uint8_t event, uint8_t * data);
unsigned char _gantimer_state = TIMER_STATE_DISCONNECTED;

// ----------------------------------------------------------------------------
// 
// ----------------------------------------------------------------------------

unsigned char gantimer_state() {
    return _gantimer_state;
}

bool gantimer_validate_data(uint8_t* data, uint16_t len) {

    if (0 == len) return false;
    if (0xfe != data[0]) return false;

    // TODO: CRC validation

    return true;

}

void gantimer_data_callback(uint8_t* data, uint16_t len) {

    // Validate data
    if (!gantimer_validate_data(data, len)) return;

    // Decode data
    unsigned char state = data[3];

    // Start
    if (3 == state) {
        if (_gantimer_state != TIMER_STATE_RUNNING) {
            _gantimer_state = TIMER_STATE_RUNNING;
            _gantimer_callback(TIMER_EVENT_START, nullptr);
        }
    }
    if (4 == state) {
        unsigned char min = data[4];
        unsigned char sec = data[5];
        unsigned int msec = (data[7] * 256) + data[6];
        unsigned long ms = min * 60000 + sec * 1000 + msec;
        uint8_t data[4] = {0};
        data[0] = (ms >> 24) & 0xFF;
        data[1] = (ms >> 16) & 0xFF;
        data[2] = (ms >>  8) & 0xFF;
        data[3] = (ms >>  0) & 0xFF;
        _gantimer_state = TIMER_STATE_STOPPED;
        _gantimer_callback(TIMER_EVENT_STOP, data);
    }
    if (5 == state) {
        _gantimer_state = TIMER_STATE_WAITING;
        _gantimer_callback(TIMER_EVENT_RESET, nullptr);  
    }

}

// ----------------------------------------------------------------------------
// Bluetooth
// ----------------------------------------------------------------------------

const uint8_t GANTIMER_UUID_SERVICE_DATA[] = { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xF0, 0xFF, 0x00, 0x00 };
const uint8_t GANTIMER_UUID_CHARACTERISTIC_DATA[] = { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xF5, 0xFF, 0x00, 0x00 };

BLEClientService _gantimer_service_data(GANTIMER_UUID_SERVICE_DATA);
BLEClientCharacteristic _gantimer_characteristic_data(GANTIMER_UUID_CHARACTERISTIC_DATA);

bool gantimer_start(uint16_t conn_handle) {

     // Discover GanTimer data service (only one supported right now)
    if ( !_gantimer_service_data.discover(conn_handle) ) {
        #if DEBUG > 0
            Serial.println("[GII] GANTIMER data service not found. Skipping.");
        #endif
        return false;
    }
    #if DEBUG > 0
        Serial.println("[GII] GANTIMER data service found.");
    #endif

    // Discover GanTimer data characteristic
    if ( ! _gantimer_characteristic_data.discover() ) {
        #if DEBUG > 0
            Serial.println("[GII] GANTIMER data characteristic not found. Skipping.");
        #endif
        return false;
    }
    _gantimer_characteristic_data.enableNotify();
    #if DEBUG > 0
        Serial.println("[GII] GANTIMER data characteristic found. Subscribed.");
    #endif

    // Send connect callback
    _gantimer_state = TIMER_STATE_WAITING;
    if (_gantimer_callback) _gantimer_callback(TIMER_EVENT_CONNECT, nullptr);

    return true;
    
}

void gantimer_stop() {
    // TODO: should this check if connected?
    _gantimer_state = TIMER_STATE_DISCONNECTED;
    if (_gantimer_callback) _gantimer_callback(TIMER_EVENT_DISCONNECT, nullptr);
}

void gantimer_set_callback(void (*callback)(uint8_t event, uint8_t * data)) {
    _gantimer_callback = callback;
}

void gantimer_init() {

    // Init objects
    _gantimer_service_data.begin();
    _gantimer_characteristic_data.begin();
    
    // Notifications
    _gantimer_characteristic_data.setNotifyCallback([](BLEClientCharacteristic* chr, uint8_t* data, uint16_t len) {
        gantimer_data_callback(data, len);
    });

}
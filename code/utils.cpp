#include <Arduino.h>

#include "config.h"
#include "system.h"
#include "bluetooth.h"
#include "display.h"
#include "touch.h"
#include "cube.h"
#include "utils.h"

char _utils_device_name[32] = {0};
char _utils_peer_version[10] = {0};
uint8_t _utils_peer_battery = 0;

void utils_setup() {
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(BAT_MEASUREMENT_GPIO, INPUT);
    analogReference(AR_INTERNAL_3_0);
    analogReadResolution(12);
}

unsigned char utils_get_battery() {
    float raw = analogRead(BAT_MEASUREMENT_GPIO);
    unsigned long mv = raw * REAL_VBAT_MV_PER_LSB;
    #if DEBUG > 2
        Serial.printf("[MAIN] Battery reading: %d\n", mv);
    #endif
    mv = constrain(mv, BATT_MIN_MV, BATT_MAX_MV);
    return map(mv, BATT_MIN_MV, BATT_MAX_MV, 0, 100);
}

void utils_beep(unsigned short freq, unsigned long ms) {
    tone(BUZZER_PIN, freq);
    system_delay(ms);
    noTone(BUZZER_PIN);
}

char * utils_device_name() {

    if (_utils_device_name[0] == 0) {
        unsigned char * addr = bluetooth_local_addr();
        snprintf(_utils_device_name, sizeof(_utils_device_name), "%s_%02X%02X%02X", 
            APP_NAME, addr[3], addr[4], addr[5]
        );

    }

    return _utils_device_name;

}

uint8_t utils_get_bit(uint8_t * data, uint16_t position) {
    uint8_t byte = position >> 3;
    uint8_t bit = 7 - (position & 0x07);
    uint8_t value = (data[byte] >> bit) & 0x01;
    return value;
}

uint16_t utils_get_bits(uint8_t * data, uint16_t position, uint8_t len) {
    uint16_t value = 0;
    for (uint8_t i=0; i<len; i++) {
        uint8_t bit = position + i;
        value = (value << 1) + utils_get_bit(data, bit);
    }
    return value;
}

float utils_tps(uint32_t time, uint16_t turns) {
    return (float) turns / ((float) time / 1000.0);
}

char * utils_time_to_text(uint32_t time, char * buffer, bool zeropad) {
    uint16_t ms = time % 1000;
    time /= 1000;
    uint8_t sec = time % 60;
    time /= 60;
    uint8_t min = time;
    sprintf(buffer, "%02d:%02d.%03d", min, sec, ms);
    if (!zeropad) {
        for (uint8_t i=0; i<4; i++) {
            if ((buffer[i] != '0') & (buffer[i] != ':')) break;
            buffer[i] = ' ';
        }
    }
    return buffer;
}
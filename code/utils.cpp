#include <Arduino.h>

#include "config.h"
#include "bluetooth.h"
#include "display.h"
#include "touch.h"
#include "cube.h"
#include "utils.h"

char _utils_device_name[32] = {0};
char _utils_peer_version[10] = {0};
bool _utils_sleeping = false;
uint8_t _utils_peer_battery = 0;

void wdt_set(unsigned long seconds) {
    NRF_WDT->CONFIG         = 0x00;                 // Configure WDT to run when CPU is asleep
    NRF_WDT->CRV            = 32768 * seconds + 1;  // CRV = timeout * 32768 + 1
    NRF_WDT->RREN           = 0x01;                 // Enable the RR[0] reload register
    NRF_WDT->TASKS_START    = 1;                    // Start WDT       
}

void wdt_feed() {
    NRF_WDT->RR[0] = WDT_RR_RR_Reload;
}

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

void utils_delay(uint32_t ms) {
    uint32_t start = millis();
    while (millis() - start < ms) delay(1);
}

void utils_beep(unsigned short freq, unsigned long ms) {
    tone(BUZZER_PIN, freq);
    utils_delay(ms);
    noTone(BUZZER_PIN);
}

bool utils_sleeping() {
    return _utils_sleeping;
}

void utils_sleep() {

    _utils_sleeping = true;

    display_clear();
    display_off();
    bluetooth_scan(false);
    bluetooth_disconnect();

    sd_power_mode_set(NRF_POWER_MODE_LOWPWR);

    attachInterrupt(
        TOUCH_INT_PIN, 
        []() { if (_utils_sleeping) _utils_sleeping = false; },
        RISING
    );

    #if DEBUG > 0
        Serial.printf("[MAIN] Goind to sleep\n");
        delay(100);
    #endif

    NRF_UARTE0->ENABLE = 0; //disable UART
    NRF_SAADC->ENABLE = 0;  //disable ADC
    NRF_PWM0->ENABLE = 0;   //disable all pwm instance
    NRF_PWM1->ENABLE = 0;
    NRF_PWM2->ENABLE = 0;
    NRF_TWIM1->ENABLE = 0; //disable TWI Master
    NRF_TWIS1->ENABLE = 0; //disable TWI Slave
    NRF_SPI0->ENABLE = 0;  //disable SPI
    NRF_SPI1->ENABLE = 0;  //disable SPI
    NRF_SPI2->ENABLE = 0;  //disable SPI

    
    while(true) {
    
        while (_utils_sleeping) {
            __WFE();
            __WFI();
        }

        // get out of sleeping if pressed for > 1s
        delay(1000);
        if (digitalRead(TOUCH_INT_PIN) == LOW) break;
        
    }

    detachInterrupt(TOUCH_INT_PIN);

    NRF_UARTE0->ENABLE = UARTE_ENABLE_ENABLE_Enabled;
    NRF_SPIM2->ENABLE = (SPI_ENABLE_ENABLE_Enabled << SPI_ENABLE_ENABLE_Pos);
    NRF_TWIM0->ENABLE = (TWIM_ENABLE_ENABLE_Enabled << TWIM_ENABLE_ENABLE_Pos);

    delay(100);

    bluetooth_scan(true);
    display_on();
    touch_setup(TOUCH_INT_PIN);

    #if DEBUG > 0
        Serial.printf("[MAIN] Waked up\n");
    #endif

    _utils_sleeping = false;

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
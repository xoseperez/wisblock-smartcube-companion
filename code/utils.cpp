#include <Arduino.h>

#include "config.h"
#include "bluetooth.h"
#include "utils.h"

char _utils_device_name[32] = {0};
char _utils_peer_version[10] = {0};
bool _utils_sleeping = false;
uint8_t _utils_peer_battery = 0;

void utils_setup() {
}

bool charge_status() {
}

float battery_read() {
    return 0;
}

void wdt_set(unsigned long seconds) {
    NRF_WDT->CONFIG         = 0x00;                 // Configure WDT to run when CPU is asleep
    NRF_WDT->CRV            = 32768 * seconds + 1;  // CRV = timeout * 32768 + 1
    NRF_WDT->RREN           = 0x01;                 // Enable the RR[0] reload register
    NRF_WDT->TASKS_START    = 1;                    // Start WDT       
}

void wdt_feed() {
    NRF_WDT->RR[0] = WDT_RR_RR_Reload;
}

void sleep() {

    _utils_sleeping = true;

    sd_power_mode_set(NRF_POWER_MODE_LOWPWR);

    #ifdef DEBUG
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

    while (_utils_sleeping) {
        __WFE();
        __WFI();
    }

    NRF_UARTE0->ENABLE = UARTE_ENABLE_ENABLE_Enabled; //disable UART
    NRF_SPIM2->ENABLE = (SPI_ENABLE_ENABLE_Enabled << SPI_ENABLE_ENABLE_Pos);
    ; //disable SPI
    NRF_TWIM0->ENABLE = (TWIM_ENABLE_ENABLE_Enabled << TWIM_ENABLE_ENABLE_Pos);
    ; //disable TWI Master

    delay(100);

    #ifdef DEBUG
        Serial.printf("[MAIN] Waked up\n");
    #endif

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

void utils_set_peer_battery(uint8_t value) {
    _utils_peer_battery = value;
    #ifdef DEBUG
        Serial.printf("[MAIN] Peer battery level: %d%%\n", value);
    #endif

}

void utils_set_peer_version(char * version) {
    strncpy(_utils_peer_version, version, sizeof(_utils_peer_version));
    #ifdef DEBUG
        Serial.printf("[MAIN] Peer version: %s\n", _utils_peer_version);
    #endif

}

uint8_t utils_get_peer_battery() {
    return _utils_peer_battery;
}

char * utils_get_peer_version() {
    return _utils_peer_version;
}


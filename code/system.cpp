#include <Arduino.h>

#include "system.h"
#include "bluetooth.h"
#include "display.h"
#include "touch.h"
#include "config.h"

// ----------------------------------------------------------------------------
// Module globals
// ----------------------------------------------------------------------------

bool _system_sleeping = false;

#if defined(__CC_ARM) || (defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050))
__attribute__ ((section(".bss.uninitialised"),zero_init)) unsigned int _system_wdt_triggered;
#elif defined(__GNUC__)
__attribute__ ((section(".uninitialised"))) unsigned int _system_wdt_triggered;
#elif defined(__ICCARM__)
unsigned int _system_wdt_triggered @ ".uninitialised";
#endif

// ----------------------------------------------------------------------------
// Private methods
// ----------------------------------------------------------------------------

bool isSoftDevice() {
  uint8_t check;
  sd_softdevice_is_enabled(&check);
  return check == 1;
}

// The WDT interrupt handler will have around 2 32kHz clock cycles to execute before reset.
void WDT_IRQHandler(void) {
    _system_wdt_triggered = 1;
    NRF_WDT->EVENTS_TIMEOUT = 0;    
}

// ----------------------------------------------------------------------------
// Public methods
// ----------------------------------------------------------------------------

bool system_wdt_triggered() {
    bool output = (_system_wdt_triggered == 1);
    _system_wdt_triggered = 0;
    return output;
}

void system_wdt_enable(unsigned long seconds) {
    _system_wdt_triggered = 0;
    NRF_WDT->CONFIG         = (WDT_CONFIG_HALT_Pause << WDT_CONFIG_HALT_Pos) | ( WDT_CONFIG_SLEEP_Run << WDT_CONFIG_SLEEP_Pos);
    NRF_WDT->CRV            = 32768 * seconds + 1;
    NRF_WDT->RREN          |= WDT_RREN_RR0_Msk;
    NVIC_SetPriority(WDT_IRQn, 7);
    NVIC_ClearPendingIRQ(WDT_IRQn);
    NVIC_EnableIRQ(WDT_IRQn);
    NRF_WDT->INTENSET       = WDT_INTENSET_TIMEOUT_Msk;	    
    NRF_WDT->TASKS_START    = 1;
}

void system_wdt_disable() {
    NRF_WDT->CONFIG         = (WDT_CONFIG_HALT_Pause << WDT_CONFIG_HALT_Pos) | ( WDT_CONFIG_SLEEP_Pause << WDT_CONFIG_SLEEP_Pos);
    NRF_WDT->CRV            = 4294967295;
}

void system_wdt_feed() {
    if ((bool)(NRF_WDT->RUNSTATUS) == true) {
        NRF_WDT->RR[0] = WDT_RR_RR_Reload;
    }
}

void system_reset() {
    NVIC_SystemReset();    
}

void system_off() {

    //logWrite(LOG_DEBUG, "SYS", "Switching off");
    delay(100);

    bluetooth_scan(false);
    bluetooth_disconnect();
    display_clear();
    display_off();
    touch_power_mode(1);
    sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE);
    Serial.end();

    NRF_GPIO->PIN_CNF[TOUCH_INT_PIN] &= ~((uint32_t)GPIO_PIN_CNF_SENSE_Msk);
    NRF_GPIO->PIN_CNF[TOUCH_INT_PIN] |= ((uint32_t)GPIO_PIN_CNF_SENSE_Low << GPIO_PIN_CNF_SENSE_Pos);
    attachInterrupt(digitalPinToInterrupt(TOUCH_INT_PIN), [](){}, RISING);
  
    isSoftDevice() ? sd_power_system_off() : NRF_POWER->SYSTEMOFF = 1;

}

bool system_sleeping() {
    return _system_sleeping;
}

void system_sleep() {

    _system_sleeping = true;

    //logWrite(LOG_DEBUG, "SYS", "Going to sleep");
    delay(100);

    bluetooth_scan(false);
    bluetooth_disconnect();
    display_clear();
    display_off();
    touch_power_mode(1);
    sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE);
    Serial.end();

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
    
    isSoftDevice() ? sd_power_mode_set(NRF_POWER_MODE_LOWPWR) : NRF_POWER->TASKS_LOWPWR = 1;
    sd_app_evt_wait(); 

    while(true) {

        __WFE();
        __WFI();

        // get out of sleeping if pressed for > 2s
        delay(2000);
        if (digitalRead(TOUCH_INT_PIN) == LOW) {
            //NVIC_SystemReset();
            break;
        }

    }    

    NRF_UARTE0->ENABLE = UARTE_ENABLE_ENABLE_Enabled;
    NRF_SPIM2->ENABLE = (SPI_ENABLE_ENABLE_Enabled << SPI_ENABLE_ENABLE_Pos);
    NRF_TWIM0->ENABLE = (TWIM_ENABLE_ENABLE_Enabled << TWIM_ENABLE_ENABLE_Pos);

    display_on();
    touch_power_mode(0);
    touch_setup(TOUCH_INT_PIN);
    
    Serial.begin(115200);
    delay(100);
    //logWrite(LOG_DEBUG, "SLEEP", "Waked up");

    _system_sleeping = false;

}

void system_delay(uint32_t ms) {
    uint32_t start = millis();
    while (millis() - start < ms) delay(1);
}


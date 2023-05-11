#ifndef _CONFIG_H
#define _CONFIG_H

// ----------------------------------------------------------------------------
// General
// ----------------------------------------------------------------------------

#define APP_NAME                    "MAGIC CUBE TIMER"
#define APP_SHORT_NAME              "TIMER"
#define APP_VERSION                 "v0.6.0"
#define SHUTDOWN_TIMEOUT            60000
#define WDT_SECONDS                 30
#define SCRAMBLE_SIZE               21

// ----------------------------------------------------------------------------
// BLE
// ----------------------------------------------------------------------------

// nRF52832: -40dBm, -20dBm, -16dBm, -12dBm, -8dBm, -4dBm, 0dBm, +3dBm and +4dBm.
#define BLE_TX_POWER                -16
#define BLE_NAME                    APP_NAME
#define BLE_RSSI_FILTER             -45
#define BLE_TIMEOUT                 100

// ----------------------------------------------------------------------------
// Display
// ----------------------------------------------------------------------------

#define DISPLAY_CS_GPIO             SS
#define DISPLAY_BL_GPIO             WB_IO3
#define DISPLAY_RST_GPIO            WB_IO5
#define DISPLAY_DC_GPIO             WB_IO4
#define DISPLAY_EN_GPIO             WB_IO2

// ----------------------------------------------------------------------------
// Touch
// ----------------------------------------------------------------------------

#define TOUCH_INT_PIN               WB_IO6

// ----------------------------------------------------------------------------
// Touch
// ----------------------------------------------------------------------------

#define BUZZER_PIN                  WB_IO1
#define BUZZER_FREQ                 525
#define BUZZER_DURATION             150

// ----------------------------------------------------------------------------
// Battery
// ----------------------------------------------------------------------------

#define BAT_MEASUREMENT_GPIO        WB_A0

#define VBAT_MV_PER_LSB             (0.73242188F)   // 3.0V ADC range and 12-bit ADC resolution = 3000mV/4096
#define VBAT_DIVIDER                (0.4F)          // 1.5M + 1M voltage divider on VBAT = (1.5M / (1M + 1.5M))
#define VBAT_DIVIDER_COMP           (1.709F)        // Compensation factor for the VBAT divider
#define REAL_VBAT_MV_PER_LSB        (VBAT_DIVIDER_COMP * VBAT_MV_PER_LSB)

#define BATT_MIN_MV                 3300
#define BATT_MAX_MV                 4200

#endif // _CONFIG_H


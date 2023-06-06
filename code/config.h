#ifndef _CONFIG_H
#define _CONFIG_H

// ----------------------------------------------------------------------------
// General
// ----------------------------------------------------------------------------

#define APP_NAME                    "SMARTCUBE COMPANION"
#define APP_VERSION                 "v0.8.0"
#define SHUTDOWN_TIMEOUT            60000
#define CONNECT_TIMEOUT             20000    
#define INTRO_TIMEOUT               5000
#define WDT_SECONDS                 30
#define SCRAMBLE_SIZE               20
#define SCRAMBLE_FRU_ONLY           0

// ----------------------------------------------------------------------------
// States et al.
// ----------------------------------------------------------------------------

enum {
    STATE_SLEEPING,
    STATE_INTRO,
    STATE_CONFIG,
    STATE_SMARTCUBE_CONNECT,
    STATE_STACKMAT_CONNECT,
    STATE_PUZZLES,
    STATE_USER,
    STATE_USER_CONFIRM_RESET,
    STATE_2D,
    STATE_3D,
    STATE_SCRAMBLE,
    STATE_SCRAMBLE_MANUAL,
    STATE_INSPECT,
    STATE_READY,
    STATE_TIMER,
    STATE_SOLVED
};

enum {
    MODE_SMARTCUBE,
    MODE_STACKMAT,
    MODE_MANUAL,
    MODE_SHUTDOWN,
    MODE_NONE
};

enum {
    PUZZLE_3x3x3,
    PUZZLE_2x2x2,
    PYRAMINX,
    SKEWB,
    MEGAMINX,
    MEGAMINX2
};

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
// Stackmat interface
// ----------------------------------------------------------------------------

#define STACKMAT_RX_PIN             PIN_SERIAL1_RX
#define STACKMAT_TX_PIN             PIN_SERIAL1_TX
#define STACKMAT_BAUDRATE           1200
#define STACKMAT_TIMEOUT            2000

// ----------------------------------------------------------------------------
// Touch
// ----------------------------------------------------------------------------

#define BUZZER_PIN                  WB_IO1
#define BUZZER_FREQ_OK              525
#define BUZZER_FREQ_ERROR           220
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
#define BATT_MAX_MV                 4100

#endif // _CONFIG_H


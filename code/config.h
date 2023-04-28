#ifndef _CONFIG_H
#define _CONFIG_H

// ----------------------------------------------------------------------------
// General
// ----------------------------------------------------------------------------

#define APP_NAME            "RAKcube"
#define APP_VERSION         "v0.1.0"
#define DISPLAY_TIMEOUT     10000
#define WDT_SECONDS         30

#define TIME_OFFSET_WINTER  3600 // GMT+1
#define TIME_OFFSET_SUMMER  7200 // GMT+2

// ----------------------------------------------------------------------------
// BLE
// ----------------------------------------------------------------------------

// nRF52832: -40dBm, -20dBm, -16dBm, -12dBm, -8dBm, -4dBm, 0dBm, +3dBm and +4dBm.
#define BLE_TX_POWER        -16
#define BLE_NAME            "RAKcube"
#define BLE_RSSI_FILTER     -50
#define BLE_TIMEOUT         1000
#define BLE_CHECK_INTE

// ----------------------------------------------------------------------------
// Touch
// ----------------------------------------------------------------------------

#define TOUCH_INT_PIN       WB_IO6

#endif // _CONFIG_H


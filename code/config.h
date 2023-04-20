#ifndef _CONFIG_H
#define _CONFIG_H

// MAC cube: AB:12:34:5F:AE:82
// UUIDs:
//  00001800-0000-1000-8000-00805f9b34fb Generic Access
//  00001801-0000-1000-8000-00805f9b34fb Generic Attribute
//  6e400001-b5a3-f393-e0a9-e50e24dc4179 Propietari
//  f95a48e6-a721-11e9-a2a3-022ae2dbcce4 Propietari
// Manufacturer data: {1281: [246, 240, 238, 130, 174, 95, 52, 18, 171]}
// References:
// * https://www.reddit.com/r/Cubers/comments/fcy4wn/reverse_engineering_gan_356i_play/


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
// State
// ----------------------------------------------------------------------------

enum states_t {
    STATE_IDLE,
    STATE_CONNECTED,
    STATE_ACTIVE
};

// ----------------------------------------------------------------------------
// Pages
// ----------------------------------------------------------------------------

enum pages_t {
    PAGE_CLOCK,
    PAGE_INFO,
    PAGE_CONNECT,
    PAGE_DISCONNECT,
    PAGE_TRACKER,
    PAGE_LOST,
    PAGE_MOVEMENT,
    PAGE_ACTIVATE,
    PAGE_DEACTIVATE,
    PAGE_UNLOCK,
    PAGE_LOCK,
    PAGE_OFF,
    PAGE_SLEEP,
    PAGE_BLACK
};

// ----------------------------------------------------------------------------
// BLE
// ----------------------------------------------------------------------------

// nRF52832: -40dBm, -20dBm, -16dBm, -12dBm, -8dBm, -4dBm, 0dBm, +3dBm and +4dBm.
#define BLE_TX_POWER        -16
#define BLE_NAME            "RAKcube"
#define BLE_RSSI_FILTER     -50
#define BLE_TIMEOUT         5000
#define BLE_CHECK_INTE

// ----------------------------------------------------------------------------

#endif // _CONFIG_H


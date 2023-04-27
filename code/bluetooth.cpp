#include <Arduino.h>
#include <bluefruit.h>

#include "config.h"
#include "bluetooth.h"
#include "ganv2.h"

#define MACS_MAX 20
unsigned char mac_values[MACS_MAX][6];
unsigned long mac_time[MACS_MAX] = {0};
unsigned short mac_count = 0;

uint16_t _bluetooth_handle = 0;
bool _bluetooth_connected = false;
unsigned char _bluetooth_local_addr[6]; // MAC of the current device
char _bluetooth_peer_name[32]; // Name of the cube we are connected to 
unsigned char _bluetooth_peer_addr[6]; // MAC of the cube we are connected to

// ----------------------------------------------------------------------------
// Bluetooth utils
// ----------------------------------------------------------------------------

unsigned char * bluetooth_local_addr() {
    return _bluetooth_local_addr;
}

unsigned char * bluetooth_peer_addr() {
    return _bluetooth_peer_addr;
}

char * bluetooth_peer_name() {
    return _bluetooth_peer_name;
}

bool bluetooth_connected() {
    return _bluetooth_connected;
}

// ----------------------------------------------------------------------------
// Bluetooth callbacks
// ----------------------------------------------------------------------------

void bluetooth_scan_callback(ble_gap_evt_adv_report_t* report) {

    // Get device MAC
    char mac[6];
    for (unsigned char i=0; i<6; i++) {
        mac[i] = report->peer_addr.addr[5-i];
    }
    
    // Check if we have already seen it in the last BLE_TIMEOUT ms
    bool found = false;
    unsigned char slot = 0;
    unsigned char free_slot = MACS_MAX;
    for (slot=0; slot<MACS_MAX; slot++) {
        if ((0 == mac_time[slot]) || (millis() - mac_time[slot] > BLE_TIMEOUT)) {
            if (free_slot == MACS_MAX) free_slot = slot;
            continue;
        }
        if (memcmp(mac_values[slot], mac, 6) == 0) {
            found = true;
            mac_time[slot] = millis();
            break;
        }
    }

    // If not found add it and report it
    if ((!found) && (free_slot < MACS_MAX)) {

        // Add it to the list
        memcpy(mac_values[free_slot], mac, 6);
        mac_time[free_slot] = millis();
        
        #if DEBUG > 0

            Serial.printf("[BLE] Packet from ");
            
            // MAC is in little endian --> print reverse
            Serial.printBufferReverse(report->peer_addr.addr, 6, ':');

            // RSSI
            Serial.printf(", RSSI: %d dBm", report->rssi);

            // Device name
            memset(_bluetooth_peer_name, 0, sizeof(_bluetooth_peer_name));
            if (Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME, (uint8_t*) _bluetooth_peer_name, sizeof(_bluetooth_peer_name))) {
                Serial.printf("%14s %s\n", ", complete name: ", _bluetooth_peer_name);
            } else if (Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME, (uint8_t*) _bluetooth_peer_name, sizeof(_bluetooth_peer_name))) {
                Serial.printf("%14s %s\n", ", short name: ", _bluetooth_peer_name);
            } else {
                Serial.printf("\n");
            }
        
        #endif

        // Connect to device
        Bluefruit.Central.connect(report);

    }
   

}

void bluetooth_connect_callback(uint16_t conn_handle) {
    
    // Get the reference to current connection
    BLEConnection* connection = Bluefruit.Connection(conn_handle);
    connection->getPeerName(_bluetooth_peer_name, sizeof(_bluetooth_peer_name));
    ble_gap_addr_t ble_addr = connection->getPeerAddr();
    _bluetooth_peer_addr[0] = ble_addr.addr[5];
    _bluetooth_peer_addr[1] = ble_addr.addr[4];
    _bluetooth_peer_addr[2] = ble_addr.addr[3];
    _bluetooth_peer_addr[3] = ble_addr.addr[2];
    _bluetooth_peer_addr[4] = ble_addr.addr[1];
    _bluetooth_peer_addr[5] = ble_addr.addr[0];
    
    #if DEBUG > 0
        Serial.printf("[BLE] Connected to %02X:%02X:%02X:%02X:%02X:%02X (%s)\n", 
            _bluetooth_peer_addr[0],_bluetooth_peer_addr[1],_bluetooth_peer_addr[2],
            _bluetooth_peer_addr[3],_bluetooth_peer_addr[4],_bluetooth_peer_addr[5],
            _bluetooth_peer_name
        );
    #endif

    if (ganv2_start(conn_handle)) {
        _bluetooth_handle = conn_handle;
        _bluetooth_connected = true;
    } else {
        connection->disconnect();
        bluetooth_scan(true);
    }

}

void bluetooth_disconnect_callback(uint16_t conn_handle, uint8_t reason) {
  
    (void) conn_handle;
    (void) reason;
  
    #if DEBUG > 0
        // BLE_HCI_LOCAL_HOST_TERMINATED_CONNECTION (0x16) is a normal termination 
        Serial.printf("[BLE] Disconnected, reason 0x%02X\n", reason);
    #endif

    _bluetooth_connected = false;
    ganv2_stop();

}

// ----------------------------------------------------------------------------
// Main bluetooth methods
// ----------------------------------------------------------------------------

void bluetooth_scan(bool scan) {

    if (scan) {
        
        /* Start Central Scanning
        * - Enable auto scan if disconnected
        * - Filter out packet with a min rssi
        * - Interval = 100 ms, window = 50 ms
        * - Use active scan (used to retrieve the optional scan response adv packet)
        * - Start(0) = will scan forever since no timeout is given
        */
        Bluefruit.Scanner.setRxCallback(bluetooth_scan_callback);
        Bluefruit.Scanner.restartOnDisconnect(true);
        Bluefruit.Scanner.filterRssi(BLE_RSSI_FILTER);
        Bluefruit.Scanner.setInterval(160, 80);       // in units of 0.625 ms
        Bluefruit.Scanner.useActiveScan(false);        // Request scan response data
        Bluefruit.Scanner.start(0);                   // 0 = Don't stop scanning after n seconds

        #if DEBUG > 0
            Serial.println("[BLE] Start scanning");
        #endif

    } else {
        
        Bluefruit.Scanner.stop();

        #if DEBUG > 0
            Serial.println("[BLE] Stop scanning");
        #endif

    }
    
}

void bluetooth_disconnect() {
    if (!_bluetooth_connected) return;
    BLEConnection* conn = Bluefruit.Connection(_bluetooth_handle);
    conn->disconnect();
}

void bluetooth_setup() {

    // Initialize Bluefruit with maximum connections as Peripheral = 0, Central = 1
    // SRAM usage required by SoftDevice will increase dramatically with number of connections
    Bluefruit.begin(0, 1);
    Bluefruit.setTxPower(BLE_TX_POWER);    // Check bluefruit.h for supported values

    // Get BLE address
    uint32_t addr_high = ((MAC_ADDRESS_HIGH) & 0x0000ffff) | 0x0000c000;
    uint32_t addr_low  = MAC_ADDRESS_LOW;
    _bluetooth_local_addr[0] = (addr_high >>  8) & 0xFF;
    _bluetooth_local_addr[1] = (addr_high >>  0) & 0xFF;
    _bluetooth_local_addr[2] = (addr_low  >> 24) & 0xFF;
    _bluetooth_local_addr[3] = (addr_low  >> 16) & 0xFF;
    _bluetooth_local_addr[4] = (addr_low  >>  8) & 0xFF;
    _bluetooth_local_addr[5] = (addr_low  >>  0) & 0xFF;
    #if DEBUG > 0
        Serial.printf("[BLE] Device addr: %02X:%02X:%02X:%02X:%02X:%02X\n", 
            _bluetooth_local_addr[0], _bluetooth_local_addr[1], _bluetooth_local_addr[2],
            _bluetooth_local_addr[3], _bluetooth_local_addr[4], _bluetooth_local_addr[5]
        );
    #endif

    // Set the BLE device name
    Bluefruit.setName("RAKcube");

    // Callbacks for Central
    Bluefruit.Central.setConnectCallback(bluetooth_connect_callback);
    Bluefruit.Central.setDisconnectCallback(bluetooth_disconnect_callback);

    /* Set the LED interval for blinky pattern on BLUE LED */
    Bluefruit.setConnLedInterval(250);

}

void bluetooth_loop() {

}

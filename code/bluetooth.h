#ifndef _BLUETOOTH_H
#define _BLUETOOTH_H

#include <bluefruit.h>

typedef volatile uint32_t REG32;
#define pREG32 (REG32 *)
#define DEVICE_ID_HIGH    (*(pREG32 (0x10000060)))
#define DEVICE_ID_LOW     (*(pREG32 (0x10000064)))
#define MAC_ADDRESS_HIGH  (*(pREG32 (0x100000a8)))
#define MAC_ADDRESS_LOW   (*(pREG32 (0x100000a4)))

unsigned char * bluetooth_local_addr();
unsigned char * bluetooth_peer_addr();
char * bluetooth_peer_name();
bool bluetooth_discover_service(BLEClientService service, const char * header, const char * name);
bool bluetooth_discover_characteristic(BLEClientCharacteristic characteristic, const char * header, const char * name);

void bluetooth_setup();
void bluetooth_loop();
void bluetooth_scan(bool);
bool bluetooth_connected();
void bluetooth_disconnect();

#endif // _BLUETOOTH_H

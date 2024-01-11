#include <LoRaWan-RAK4630.h>
#include "lorawan.h"

void lorawan_setup() {
    lora_rak4630_init();
	Radio.Standby();
	Radio.Sleep();
}
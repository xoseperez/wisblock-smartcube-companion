#include "config.h"
#include "bluetooth.h"
#include "gan.h"
#include "utils.h"

void setup() {
    
    #ifdef DEBUG
        Serial.begin(115200);
		while ((!Serial) && (millis() < 2000)) delay(10);
		Serial.printf("\n[MAIN] RAKcube\n\n");
    #endif

    wdt_set(WDT_SECONDS);
    
    bluetooth_setup();
    bluetooth_scan(true);

}

void loop() {

    bluetooth_loop();
    wdt_feed();
    delay(50);

}

#include <Arduino.h>
#include <SPI.h>

#include "config.h"
#include "bluetooth.h"
#include "utils.h"
#include "display.h"
#include "touch.h"
#include "cube.h"

void touch_callback(unsigned char event) {
    
    if (event == FT6336U_EVENT_SWIPE_DOWN) {
        if (bluetooth_connected()) {
            if (cube_status() == 3) {
                cube_reset();
                display_hide_timer();
            } else {
                Serial.println("[MAIN] Disconnnect request");
                bluetooth_disconnect();
            }
        } else {
            Serial.println("[MAIN] Shutdown request");
            sleep();
        }
    }

}

void setup() {
    
    #ifdef DEBUG
        Serial.begin(115200);
		while ((!Serial) && (millis() < 2000)) delay(10);
		Serial.printf("\n[MAIN] RAKcube\n\n");
    #endif

    wdt_set(WDT_SECONDS);
    
    utils_setup();
    bluetooth_setup();
    bluetooth_scan(true);
    display_setup();
    cube_setup();

    while (!touch_setup(TOUCH_INT_PIN)) {
	    Serial.println("[MAIN] Touch interface is not connected");
		delay(1000);
    }

    // Callback to be called upn event
    touch_set_callback(touch_callback);

}

void loop() {

    bluetooth_loop();
    display_loop();
    touch_loop();

    wdt_feed();
    delay(50);

}

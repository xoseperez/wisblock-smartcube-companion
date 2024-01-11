#include <Arduino.h>

#include "config.h"
#include "timer.h"
#include "timers/stackmat.h"
#include "timers/gantimer.h"

uint8_t _timer_battery = 0xFF;


void timer_setup() {
    stackmat_init();
    gantimer_init();
}

void timer_enable(bool enable) {
    stackmat_enable(enable);
}

uint8_t timer_state() {

    uint8_t state;
    
    state = gantimer_state();
    if (TIMER_STATE_DISCONNECTED != state) return state;

    state = stackmat_state();
    return state;

}

void timer_set_callback(void (*callback)(uint8_t event, uint8_t * data)) {
    stackmat_set_callback(callback);
    gantimer_set_callback(callback);
}

bool timer_bind(uint8_t conn_handle) {
    return gantimer_start(conn_handle);
}

void timer_unbind() {
    gantimer_stop();
}

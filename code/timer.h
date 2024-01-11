#ifndef _TIMER_H
#define _TIMER_H

enum {
    TIMER_EVENT_CONNECT,
    TIMER_EVENT_DISCONNECT,
    TIMER_EVENT_START,
    TIMER_EVENT_STOP,
    TIMER_EVENT_RESET
};

enum {
    TIMER_STATE_DISCONNECTED,
    TIMER_STATE_WAITING,
    TIMER_STATE_RUNNING,
    TIMER_STATE_STOPPED,
};

// ------------------------------------

void timer_setup();
void timer_enable(bool enable = false);
uint8_t timer_state();
void timer_set_callback(void (*callback)(uint8_t event, uint8_t * data));

// ------------------------------------
// Callbacks from smart timers
// ------------------------------------

// ------------------------------------
// Callbacks from bluetooth module
// ------------------------------------

bool timer_bind(uint8_t conn_handle);
void timer_unbind();

#endif // _TIMER_H

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

typedef union {
  float number = 0;
  uint8_t bytes[4];
} FLOATUNION_t;

void timer_loop();
void timer_setup();
void timer_set_callback(void (*callback)(uint8_t event, uint8_t * data));
uint8_t timer_state();

#endif // _TIMER_H

#ifndef _STACKMAT_H
#define _STACKMAT_H

enum {
    STACKMAT_EVENT_CONNECT,
    STACKMAT_EVENT_DISCONNECT,
    STACKMAT_EVENT_START,
    STACKMAT_EVENT_STOP,
    STACKMAT_EVENT_RESET
};

enum {
    STACKMAT_STATE_DISCONNECTED,
    STACKMAT_STATE_WAITING,
    STACKMAT_STATE_RUNNING,
    STACKMAT_STATE_STOPPED,
};

typedef union {
  float number = 0;
  uint8_t bytes[4];
} FLOATUNION_t;

void stackmat_loop();
void stackmat_setup();
void stackmat_set_callback(void (*callback)(uint8_t event, uint8_t * data));
uint8_t stackmat_state();

#endif // _STACKMAT_H

#include "RAK14014_FT6336U.h"

#ifndef _TOUCH_H
#define _TOUCH_H

enum {
    TOUCH_EVENT_CLICK = 0x01,
    TOUCH_EVENT_PRESS = 0x01,
    TOUCH_EVENT_HOLD = 0x02,
    TOUCH_EVENT_DOUBLE_CLICK = 0x03, // Not implemented
    TOUCH_EVENT_RELEASE = 0x04,
    TOUCH_EVENT_CLICK_TWO_FINGERS = 0x05,
    TOUCH_EVENT_DOUBLE_CLICK_TWO_FINGERS = 0x06, // Not implemented
    TOUCH_EVENT_SWIPE_LEFT = 0x07,
    TOUCH_EVENT_SWIPE_RIGHT = 0x08,
    TOUCH_EVENT_SWIPE_UP = 0x09,
    TOUCH_EVENT_SWIPE_DOWN = 0x0A,
};

bool touch_setup(unsigned char interrupt_pin);
void touch_loop(void);
unsigned char touch_event();
void touch_set_callback(void (*callback)(unsigned char type));
TouchPointType touch_pointA();
TouchPointType touch_pointB();

#endif // _TOUCH_H

#include "RAK14014_FT6336U.h"

#ifndef _TOUCH_H
#define _TOUCH_H

enum {
    TOUCH_EVENT_CLICK,
    TOUCH_EVENT_HOLD,
    TOUCH_EVENT_LONG_CLICK,
    TOUCH_EVENT_DOUBLE_CLICK, // Not implemented
    TOUCH_EVENT_RELEASE,
    TOUCH_EVENT_CLICK_TWO_FINGERS,
    TOUCH_EVENT_DOUBLE_CLICK_TWO_FINGERS, // Not implemented
    TOUCH_EVENT_SWIPE_LEFT,
    TOUCH_EVENT_SWIPE_RIGHT,
    TOUCH_EVENT_SWIPE_UP,
    TOUCH_EVENT_SWIPE_DOWN,
};

bool touch_setup(unsigned char interrupt_pin);
void touch_loop(void);
unsigned char touch_event();
void touch_set_callback(void (*callback)(unsigned char type));
TouchPointType touch_pointA();
TouchPointType touch_pointB();

#endif // _TOUCH_H

#ifndef _STACKMAT_H
#define _STACKMAT_H

#include "timer.h"

void stackmat_init();
void stackmat_enable(bool enable = false);
uint8_t stackmat_state();
void stackmat_set_callback(void (*callback)(uint8_t event, uint8_t * data));

#endif // _STACKMAT_H

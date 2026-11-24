#ifndef TIMER_H_
#define TIMER_H_

#include <stdint.h>
#include "../ProjectDefine/ProjectDefines.h"

extern volatile uint8_t timer_second_flag;
extern volatile uint32_t timer_seconds;

void Timer1_Init_ms(uint16_t timeout_ms);

#endif

#ifndef __DELAY_H__
#define __DELAY_H__

#include "stm32f4xx.h"

void delay_us(uint16_t nus);
void delay_ms(u16 nms);
void systick_delay_us(uint32_t nus);

#endif

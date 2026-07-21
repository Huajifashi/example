#ifndef __IR_SEND_H__
#define __IR_SEND_H__
#include "stm32f4xx.h"

void IR_Send_Init(void);
void IR_Send(uint8_t addr, uint8_t code);

#endif /* __IR_SEND_H__ */
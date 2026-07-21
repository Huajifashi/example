#ifndef __CAN_H__
#define __CAN_H__
#include "stm32f4xx.h"

void CAN1_Init(void);
void CAN1_SendMsg(uint32_t uID, uint8_t *pData, uint32_t uLen);

#endif
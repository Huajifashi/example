#ifndef __FPR_ZN632_H__
#define __FPR_ZN632_H__

#include <stdio.h>
#include "stm32f4xx.h"

extern uint32_t ZN632_RxBufLen;
extern uint8_t ZN632_RxBuf[];

void FPR_Init(uint32_t baud);
void FPR_AddFinger(void);
void FPR_MatchFinger(void);
void FPR_DeleteFinger(void);

#define ZN632_PowerOff() GPIO_SetBits(GPIOC, GPIO_Pin_9)
int16_t ZN632_PowerOn(void);

int16_t ZN632_VryPwd(void);
int16_t ZN632_ReadIndexTable(void);
uint16_t ZN632_GetIndexEmpty(void);
void ZN632_SetIndex(uint16_t uIndex);

int16_t ZN632_GetImage(void);
int16_t ZN632_GenChar(uint8_t BufferID);
int16_t ZN632_Match(void);
int16_t ZN632_RegModel(void);
int16_t ZN632_StoreChar(uint8_t BufferID, uint16_t PageID);
int16_t ZN632_HighSpeedSearch(uint8_t BufferID, uint16_t *pID, uint16_t *pScore);
int16_t ZN632_DeletChar(uint16_t PageID, uint16_t N);
int16_t ZN632_Empty(void);

#endif
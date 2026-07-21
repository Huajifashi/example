#ifndef __E2PROM_AT24C02_H__
#define __E2PROM_AT24C02_H__

#include "stm32f4xx.h"

void AT24C02_Init(void);
void AT24C02_BufferWrite(uint8_t* pBuffer, uint8_t WriteAddr, uint16_t Number);
int8_t AT24C02_ByteWrite(uint8_t* pBuffer, uint8_t WriteAddr);
int8_t AT24C02_PageWrite(uint8_t* pBuffer, uint8_t WriteAddr, uint8_t Number);
int8_t AT24C02_BufferRead(uint8_t* pBuffer, uint8_t ReadAddr, uint16_t Number);

#endif /* __E2PROM_AT24C02_H__ */
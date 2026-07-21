#ifndef __DHT11_H__
#define    __DHT11_H__

#include "stm32f4xx.h"

typedef struct{
    uint8_t  humi_int;        //湿度的整数部分
    uint8_t  humi_deci;         //湿度的小数部分
    uint8_t  temp_int;         //温度的整数部分
    uint8_t  temp_deci;         //温度的小数部分
    uint8_t  check_sum;         //校验和
} DHT11_Data;


void   DHT11_Init( void );
int8_t DHT11_ReadData(DHT11_Data *pData);

#endif /* __DHT11_H__ */
#ifndef __GPS_H__
#define __GPS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stm32f4xx.h"

#include "gps_parser.h"

void GPS_Init(uint32_t baud);
void GPS_ReadAndParse(void);

#endif //__GPS_H__
#ifndef _RTC_H_
#define _RTC_H_
#include "stm32f4xx.h"

ErrorStatus RTC_Set_Date(u8 year,u8 month,u8 date,u8 week);
ErrorStatus RTC_Set_Time(u8 hour,u8 min,u8 sec,u8 ampm);

#endif
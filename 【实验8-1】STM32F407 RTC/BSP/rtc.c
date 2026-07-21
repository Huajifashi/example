#include "rtc.h"
#include "delay.h" //简单延时

u8 BSP_RTC_Init(void)
{
RTC_InitTypeDef RTC_InitStructure;
u16 retry = 0X1FFF; 
RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);  //使能PWR时钟
PWR_BackupAccessCmd(ENABLE);       //使能后备寄存器访问 
    
/*根据BKP寄存器中的值,判断是否是第一次设置 RTC? (或者没有RTC电池)*/
if(RTC_ReadBackupRegister(RTC_BKP_DR0) != 0x5050)    
{
    RCC_LSEConfig(RCC_LSE_ON);    //开启LSE     
    /*检查指定的RCC标志位设置与否,等待低速晶振就绪*/
    while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)    
    {
        retry++;
        delay_ms(10);
    }
    if(retry==0)return 1;//LSE 开启失败. 返回1
    /*设置RTC时钟(RTCCLK),选择LSE作为RTC时钟*/
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
    RCC_RTCCLKCmd(ENABLE);     //使能RTC时钟  

    RTC_InitStructure.RTC_AsynchPrediv = 0x7F;//RTC异步分频系数
    RTC_InitStructure.RTC_SynchPrediv  = 0xFF;//RTC同步分频系数
    RTC_InitStructure.RTC_HourFormat  = RTC_HourFormat_24;//24小时格式
    RTC_Init(&RTC_InitStructure);
 
    RTC_Set_Time(13,00,00,RTC_H12_AM);     //设置时间
    RTC_Set_Date(21,12,15,3);    //设置日期
    /*已经配置好,保存标志到RTC_BKP_DR0(电池保存)*/
    RTC_WriteBackupRegister(RTC_BKP_DR0,0x5050);    
} 
return 0;
}

ErrorStatus RTC_Set_Time(u8 hour,u8 min,u8 sec,u8 ampm)  //设置时间
{
    RTC_TimeTypeDef RTC_TimeTypeInitStructure;

    RTC_TimeTypeInitStructure.RTC_Hours = hour;
    RTC_TimeTypeInitStructure.RTC_Minutes = min;
    RTC_TimeTypeInitStructure.RTC_Seconds = sec;
    RTC_TimeTypeInitStructure.RTC_H12 = ampm;
    return RTC_SetTime(RTC_Format_BIN, &RTC_TimeTypeInitStructure);
}
 
ErrorStatus RTC_Set_Date(u8 year,u8 month,u8 date,u8 week)  //设置日期
{
    RTC_DateTypeDef RTC_DateTypeInitStructure;
    RTC_DateTypeInitStructure.RTC_Date = date;
    RTC_DateTypeInitStructure.RTC_Month = month;
    RTC_DateTypeInitStructure.RTC_WeekDay = week;
    RTC_DateTypeInitStructure.RTC_Year = year;
    return RTC_SetDate(RTC_Format_BIN, &RTC_DateTypeInitStructure);
}


#include "stm32f4xx.h"
#include "delay.h" //简单延时
#include "oled.h" //128*32 OLED显示驱动
#include "rtc.h"
#include "stdio.h" //包含C语言标准输入输出头文件

RTC_TimeTypeDef RTC_TimeStruct;//时间结构体
RTC_DateTypeDef RTC_DateStruct; //日期结构体
u8 Rtc_Data[50];//用于显示的临时数组

int main(void){
    delay_ms(100); //延时等待OLED上电完成
    OLED_Init(); //OLED初始化
    OLED_Clear(); //OLED清屏
    OLED_ShowCHinese(28,0,0); //显示 青
    delay_ms(200); //延时200毫秒
    OLED_ShowCHinese(46,0,1); //显示 软
    delay_ms(200); //延时200毫秒
    OLED_ShowCHinese(64,0,2); //显示 集
    delay_ms(200); //延时200毫秒
    OLED_ShowCHinese(82,0,3); //显示 团

    BSP_RTC_Init();//RTC 初始化
    delay_ms(1000);//延时1秒
    /*初始化完毕，可以进入主循环while(1)了*/

    while(1)
    {
        RTC_GetTime(RTC_Format_BIN,&RTC_TimeStruct);//获取时间
        sprintf((char*)Rtc_Data," Time:%02d:%02d:%02d ", RTC_TimeStruct.RTC_Hours, RTC_TimeStruct.RTC_Minutes, RTC_TimeStruct.RTC_Seconds);
        OLED_ShowString(0,0,Rtc_Data,16); //OLED字符串显示

        RTC_GetDate(RTC_Format_BIN, &RTC_DateStruct);//获取日期
        sprintf((char*)Rtc_Data,"Date:20%02d-%02d-%02d", RTC_DateStruct.RTC_Year, RTC_DateStruct.RTC_Month, RTC_DateStruct.RTC_Date);
        OLED_ShowString(0,2,Rtc_Data,16); //OLED字符串显示
        delay_ms(400);//400毫秒循环一次
    }
}
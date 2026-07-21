#include <stdio.h>
#include "stm32f4xx.h"
#include "xpt2046.h"
#include "usart.h"
#include "delay.h"

/**
  * @brief  主函数
  * @param  无
  * @retval 无
  */
int main(void) {
    UART2_Init(115200);   //调试串口，方便打印
    
    XPT2046_Init();
    printf("XPT2046 TEST\r\n"); 
    
    while(1){
        XPT2046_DetectTouch();
        delay_ms(50);//延时
    }
}
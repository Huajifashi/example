#include <stdio.h>
#include "stm32f4xx.h"
#include "usart.h"
#include "ir_recv.h"
#include "ir_send.h"

#define DELAY_MS_COUNT 4000

int main( void ){
    uint32_t i = 0;
    uint32_t uMSCount = 0;
    
    SysTick_Config(SystemCoreClock/1000);//初始化Systick用于精确延时
    
    UART2_Init(115200);
    IR_Recv_Init();
    printf("IR Recv Init OK\r\n");
    IR_Send_Init();
    printf("IR Send Init OK\r\n");

    while(1){
        //检测是否接收到数据
        IR_Recv();
        
        //每隔10秒发送一次
        if(uMSCount >= 10000){ 
            printf("IRSend:addr:1,code:123\r\n");
            IR_Send(1, 123);
            uMSCount = 0;
        }

        //延时计数
        if(i >= DELAY_MS_COUNT){
            uMSCount++;
            i = 0;
        }
        else{
            i++;
        }
    }
}
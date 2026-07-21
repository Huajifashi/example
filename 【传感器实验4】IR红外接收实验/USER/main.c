#include <stdio.h>
#include "stm32f4xx.h"
#include "ir_recv.h"
#include "usart.h"

int main(void){
  
    UART2_Init(115200);
    IR_Recv_Init();
    
    printf("IRRecv Init OK\r\n");
    
    while(1){
        IR_Recv();
    }
}
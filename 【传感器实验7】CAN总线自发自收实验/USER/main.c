#include "stm32f4xx.h"
#include "usart.h"
#include "delay.h"
#include "can.h"

/**
  * @brief  主函数
  * @param  无
  * @retval 无
  */
int main(void){
    uint8_t  pTxData[8] = {0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11};
    UART2_Init(115200);    
    CAN1_Init();

    while( 1 ) {
        CAN1_SendMsg(0x1234, pTxData, 8);
        delay_ms(5000);
    }
}

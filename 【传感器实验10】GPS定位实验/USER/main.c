#include <stdio.h>
#include "stm32f4xx.h"
#include "gps.h"
#include "usart.h"

int main(void)
{
    UART2_Init(115200);
    GPS_Init(9600);
    
    while(1){
        GPS_ReadAndParse();
    }
}
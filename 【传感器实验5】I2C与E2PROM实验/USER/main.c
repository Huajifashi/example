#include <stdio.h>
#include "stm32f4xx.h"
#include "e2prom_at24c02.h"
#include "usart.h"

#define TEST_SIZE 256
uint8_t BufRead[TEST_SIZE] ={0};
uint8_t BufWrite[TEST_SIZE] ={0};

void make_test_data(){
    uint32_t i = 0;

    //制造测试数据
    for(i=0; i<TEST_SIZE; i++ ){
        BufWrite[i] = i;
        printf("0x%02X ", BufWrite[i]);
        if(i%16 == 15){//分行
            printf("\r\n");
        }
    }
    printf("\r\n");
}

void check_test_data(){
    uint32_t i = 0;

    //读取数据并比较
    for(i=0; i<TEST_SIZE; i++ ){
        if(BufRead[i] != BufWrite[i]){
            printf("0x%02X ", BufRead[i]);
            printf("not correct\r\n");
            break;
        }
        printf("0x%02X ", BufRead[i]);
        if(i%16 == 15){//分行
           printf("\r\n");
        }
    }
    printf("\r\n");
    if(i >= TEST_SIZE){
        printf("TEST OK\r\n");
    }
}

int main(void){
    UART2_Init( 115200 );     
    AT24C02_Init();
    printf("AT24C02 Init OK\r\n");
    
    //写入数据
    printf("DATA Writing:\r\n");
    make_test_data();
    AT24C02_BufferWrite(BufWrite, 0, TEST_SIZE);
    printf("DATA Write OK:\r\n");

    //读出数据
    printf("DATA Reading:\r\n");
    AT24C02_BufferRead(BufRead, 0, TEST_SIZE);
    check_test_data();
    printf("DATA Read OK:\r\n");
  
    while(1){}
}
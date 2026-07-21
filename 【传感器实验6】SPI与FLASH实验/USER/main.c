#include <stdio.h>
#include "stm32f4xx.h"
#include "flash_w25qxx.h"
#include "delay.h"
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

int main( void ) {
    uint32_t uTemp = 0;
    
    UART2_Init( 115200 );     
    W25QXX_Init();
    printf("W25QXX Init OK\r\n");
    
    uTemp = W25QXX_ReadID();
    if( uTemp == 0 ){
        printf("ReadId ERROR\r\n");
    }
    printf("ReadID:[%X]\r\n", uTemp);
    
    W25QXX_SectorErase( 0 );
    printf("SectorErase OK\r\n");

    //写入数据
    printf("DATA Writing:\r\n");
    make_test_data();
    W25QXX_BufferWrite(BufWrite, 0, TEST_SIZE);
    printf("DATA Write OK:\r\n");

    //读出数据
    printf("DATA Reading:\r\n");
    W25QXX_BufferRead(BufRead, 0, TEST_SIZE);
    check_test_data();
    printf("DATA Read OK:\r\n");
  
    while(1){}
}
#include <stdio.h>
#include "stm32f4xx.h"
#include "nfc_pn532.h"
#include "usart.h"
#include "delay.h"

#define TEST_SIZE 16
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
    UART2_Init(115200);
    NFC_Init(115200);
    printf("NFC Init OK\r\n");

    NFC_WakeUp();
    make_test_data();
    while(1){
        if(NFC_Write(2, BufWrite)<0){//写入2号块
            continue;
        }
        printf("Write OK\r\n");

        delay_ms(5000);

        if(NFC_Read(2, BufRead)<0){//读出2号块
            continue;
        }
        printf("Read OK\r\n");

        check_test_data();//检查读写数据是否一致
        delay_ms(30000);
    }
}
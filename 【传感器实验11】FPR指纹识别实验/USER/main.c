#include "stm32f4xx.h"
#include "fpr_zn632.h"
#include "delay.h"
#include "usart.h"

void FPR_Usage( void ){
    printf("Select one  \r\n");
    printf("1 Add New Finger\r\n");
    printf("2 Match Finger\r\n");
}

int main(void){
    char   ch = 0;

    UART2_Init(115200);
    FPR_Init(56700);
    printf("FPR Init OK\r\n");

    if(ZN632_VryPwd() == 0){//验证模块密码
        printf("Passwrd OK\r\n");
    }
    if(ZN632_ReadIndexTable() == 0){//读取指纹索引表,确认哪些编号已注册
        printf("ReadIndexTable OK\r\n");
    }
    
    while(1){
        FPR_Usage();
        ch = getchar();
        switch( ch ){
            case '1':
                FPR_AddFinger();   //添加指纹
                break;
            case '2':
                FPR_MatchFinger();   //比对指纹
                break;
            default:
                printf("No this number\r\n");
                break;      
        }
    }
}
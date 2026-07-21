#include "stm32f4xx.h"

GPIO_InitTypeDef GPIO_InitStructure; //定义一个GPIO初始化用的结构体

/* 函数声明 */
void delay_us(uint16_t nus);
void delay_ms(u16 nms);

int main(void){
    /*开启 GPIOB 的时钟，才能使用 GPIOB*/
    RCC_AHB1PeriphClockCmd ( RCC_AHB1Periph_GPIOB, ENABLE ); //在AHB1上开启GPIOB的时钟
    /*配置三个LED所在的GPIO（PB5、PB0、PB1）*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_0 | GPIO_Pin_1; //要初始化的引脚号
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;                       //GPIO设置为输出模式
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;                      //GPIO设置为推挽模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;                   //速度设置为25MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;                    //没有上拉下拉电阻
    /*预先把输出设置为高电平，确保LED是初始化后是熄灭的*/
    GPIO_SetBits(GPIOB, GPIO_Pin_5 | GPIO_Pin_0 | GPIO_Pin_1);
    /*使用上面配置的GPIO_InitStructure数据，初始化GPIOB*/
    GPIO_Init( GPIOB, &GPIO_InitStructure );

    /*控制LED1闪烁
    while(1){
        GPIO_ResetBits(GPIOB, GPIO_Pin_5);  //点亮LED1
        delay_ms(500);                      //延时500毫秒（0.5秒）
        GPIO_SetBits(GPIOB, GPIO_Pin_5);    //熄灭LED1
        delay_ms(500);                      //延时500毫秒（0.5秒）
    }*/

    /*循环点亮LED1, 2, 3
    while(1){
        GPIO_ResetBits(GPIOB, GPIO_Pin_5);              //点亮LED1
        GPIO_SetBits(GPIOB, GPIO_Pin_0 | GPIO_Pin_1);   //熄灭LED2，3
        delay_ms(500);                                  //延时500毫秒（0.5秒）
        GPIO_ResetBits(GPIOB, GPIO_Pin_0);              //点亮LED2
        GPIO_SetBits(GPIOB, GPIO_Pin_5 | GPIO_Pin_1);   //熄灭LED1，3
        delay_ms(500);                                  //延时500毫秒（0.5秒）
        GPIO_ResetBits(GPIOB, GPIO_Pin_1);              //点亮LED3
        GPIO_SetBits(GPIOB, GPIO_Pin_5 | GPIO_Pin_0);   //熄灭LED1，2
        delay_ms(500);                                  //延时500毫秒（0.5秒）
    }*/

    /* [扩展实验：实现向左流水灯效果，仅在LED1亮时蜂鸣器鸣响] */
    //初始化GPIOA
    RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOA,ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;           //要初始化的引脚号
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;       //GPIO设置为输出模式
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;      //GPIO设置为推挽模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;   //速度设置为25MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;    //没有上拉下拉电阻
    GPIO_SetBits(GPIOA, GPIO_Pin_8);
    GPIO_Init( GPIOA, &GPIO_InitStructure );

    //编写循环
    while(1){
        GPIO_ResetBits(GPIOB, GPIO_Pin_1);              //点亮LED3
        GPIO_SetBits(GPIOA, GPIO_Pin_8);                //蜂鸣器停止工作
        GPIO_SetBits(GPIOB, GPIO_Pin_5 | GPIO_Pin_0);   //熄灭LED1，2
        delay_ms(500);                                  //延时500毫秒（0.5秒）
        GPIO_ResetBits(GPIOB, GPIO_Pin_0);              //点亮LED2
        GPIO_SetBits(GPIOB, GPIO_Pin_5 | GPIO_Pin_1);   //熄灭LED1，3
        delay_ms(500);                                  //延时500毫秒（0.5秒）
        GPIO_ResetBits(GPIOB, GPIO_Pin_5);              //点亮LED1
        GPIO_ResetBits(GPIOA, GPIO_Pin_8);              //蜂鸣器鸣响
        GPIO_SetBits(GPIOB, GPIO_Pin_0 | GPIO_Pin_1);   //熄灭LED2，3
        delay_ms(500);                                  //延时500毫秒（0.5秒）
    }
}

/***** 简单延时函数,ms,us  *****/
void delay_us(uint16_t nus){
    uint16_t i;
    while(nus--)
    {
        i = 31;
        while(i--);
    }
}

void delay_ms(u16 nms){
    uint16_t i;
    while(nms--)
    {
        i = 33800;
        while(i--);
    }
}
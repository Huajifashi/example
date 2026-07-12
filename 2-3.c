#include "stm32f4xx.h"
#define EXTRA //扩展实验，将其注释掉来运行基础实验

u8 KeyValue_1,KeyValue_2,KeyValue_3,KeyValue_4;//定义四个变量
GPIO_InitTypeDef GPIO_InitStructure; //定义一个GPIO初始化用的结构体

/* 函数声明 */
void delay_us(uint16_t nus);
void delay_ms(u16 nms);

int main(void){
    /* [配置四个按键KEY1-KEY4所在的GPIO] */
    /*开启 GPIOA GPIOC的时钟*/
    RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOC,ENABLE);

    /*配置 KEY1 所在的GPIO（PA0）*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0; //要初始化的引脚号
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;  //GPIO设置为输入模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;  //速度设置为2MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;  //上拉电阻
    /*使用上面配置的GPIO_InitStructure数据，初始化GPIOA*/
    GPIO_Init( GPIOA,  &GPIO_InitStructure );

    /*配置 KEY2，3，4 所在的GPIO（PC13，PC6，PC7）*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_6 | GPIO_Pin_7;//引脚号
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;  //GPIO设置为输入模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;  //速度设置为2MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;  //上拉电阻
    /*使用上面配置的GPIO_InitStructure数据，初始化GPIOC*/
    GPIO_Init( GPIOC,  &GPIO_InitStructure );

    /* [配置初始化三个LED，用于显示] */
    /*开启 GPIOB 的时钟，才能使用 GPIOB*/
    RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOB,ENABLE);//开启GPIOB的时钟

    /*配置三个LED所在的GPIO（PB5、PB0、PB1）*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_0 | GPIO_Pin_1;//要初始化的引脚号（PB5、PB0、PB1）
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;  //GPIO设置为输出模式
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;  //GPIO设置为推挽模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;  //速度设置为25MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;  //没有上拉下拉电阻
    /*预先把输出设置为高电平，确保LED时初始化后是熄灭的*/
    GPIO_SetBits(GPIOB, GPIO_Pin_5 | GPIO_Pin_0 | GPIO_Pin_1);
    /*使用上面配置的GPIO_InitStructure数据，初始化GPIOB*/
    GPIO_Init( GPIOB,  &GPIO_InitStructure );

#ifndef EXTRA
    /* [基础实验] */
    while(1){
        /* 读取四个按键的 GPIO值(1 or 0) */
        KeyValue_1 = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0);//读PA0的输入(KEY1)
        KeyValue_2 = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13);//读PC13的输入(KEY2)
        KeyValue_3 = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_6);//读PC6的输入(KEY3)
        KeyValue_4 = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_7);//读PC7的输入(KEY4)
        
        /*判断有无按键按下?*/
        if((0 == KeyValue_1)||(0 == KeyValue_2)||(0 == KeyValue_3)||(0 == KeyValue_4)){
            delay_ms(20);//延时20毫秒，消抖动，然后再次读取
            KeyValue_1 = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0); //读取PA0
            KeyValue_2 = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13); //读取PC13
            KeyValue_3 = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_6); //读取PC6
            KeyValue_4 = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_7); //读取PC7
                
            if(0 == KeyValue_1){//按键1 按下
                GPIO_ResetBits(GPIOB, GPIO_Pin_5);  //按下时点亮LED1
            }
            else if(0 == KeyValue_2){//按键2 按下？
                GPIO_ResetBits(GPIOB, GPIO_Pin_0);  //按下时点亮LED2
            }
            else if(0 == KeyValue_3){//按键3 按下？
                GPIO_ResetBits(GPIOB, GPIO_Pin_1);  //按下时点亮LED3
            }
            else if(0 == KeyValue_4){//按键4 按下？
                GPIO_SetBits(GPIOB, GPIO_Pin_5);  //熄灭LED1
                GPIO_SetBits(GPIOB, GPIO_Pin_0);  //       2
                GPIO_SetBits(GPIOB, GPIO_Pin_1);  //       3
            }
        }
    }
#endif

#ifdef EXTRA
    /* [扩展实验：只有抬起按键才做出相应操作] */
    while(1){
        /* 读取四个按键的 GPIO值(1 or 0) */
        KeyValue_1 = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0);//读PA0的输入(KEY1)
        KeyValue_2 = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13);//读PC13的输入(KEY2)
        KeyValue_3 = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_6);//读PC6的输入(KEY3)
        KeyValue_4 = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_7);//读PC7的输入(KEY4)

        if((0 == KeyValue_1)||(0 == KeyValue_2)||(0 == KeyValue_3)||(0 == KeyValue_4)){
            delay_ms(20);
            KeyValue_1 = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0);
            KeyValue_2 = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13);
            KeyValue_3 = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_6);
            KeyValue_4 = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_7);
                
            if(0 == KeyValue_1){
                do{
                    delay_ms(20);
                    KeyValue_1 = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0);
                }while(0 == KeyValue_1);
                GPIO_ResetBits(GPIOB, GPIO_Pin_5);
            }
            else if(0 == KeyValue_2){//按键2 按下？
                do{
                    delay_ms(20);
                    KeyValue_2 = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13);
                }while(0 == KeyValue_2);
                GPIO_ResetBits(GPIOB, GPIO_Pin_0);
            }
            else if(0 == KeyValue_3){//按键3 按下？
                do{
                    delay_ms(20);
                    KeyValue_3 = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_6);
                }while(0 == KeyValue_3);
                GPIO_ResetBits(GPIOB, GPIO_Pin_1);
            }
            else if(0 == KeyValue_4){//按键4 按下？
                do{
                    delay_ms(20);
                    KeyValue_4 = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_7);
                }while(0 == KeyValue_4);
                GPIO_SetBits(GPIOB, GPIO_Pin_5);
                GPIO_SetBits(GPIOB, GPIO_Pin_0);
                GPIO_SetBits(GPIOB, GPIO_Pin_1);
            }
        }

    }
#endif

}

/***** 简单延时函数,ms,us  *****/
void delay_us(uint16_t nus)
{
    uint16_t i;
    while(nus--)
    {
        i = 31; 
        while(i--);
    }
}
void delay_ms(u16 nms)
    {
    uint16_t i;
    while(nms--)
    {
        i = 33800;
        while(i--);
    }
}
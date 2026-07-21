#include "stm32f4xx.h"

GPIO_InitTypeDef GPIO_InitStructure; //定义一个GPIO初始化用的结构体

int main(void){
    /*开启 GPIOB 的时钟，才能使用 GPIOB*/
    RCC_AHB1PeriphClockCmd ( RCC_AHB1Periph_GPIOB, ENABLE ); //在AHB1上开启GPIOB的时钟
    /*配置三个LED所在的GPIO（PB5、PB0、PB1）*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_0 | GPIO_Pin_1; //要初始化的引脚号（PB5、PB0、PB1）
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;  //GPIO设置为输出模式
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;  //GPIO设置为推挽模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;  //速度设置为25MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;  //没有上拉下拉电阻
    
    /*预先把输出设置为高电平，确保LED是初始化后是熄灭的*/
    GPIO_SetBits(GPIOB, GPIO_Pin_5 | GPIO_Pin_0 | GPIO_Pin_1);
    
    /*使用上面配置的GPIO_InitStructure数据，初始化GPIOB*/
    GPIO_Init( GPIOB,  &GPIO_InitStructure );

    /*三个GPIO输出低电平，点亮三个LED*/
//  GPIO_ResetBits(GPIOB, GPIO_Pin_5 | GPIO_Pin_0 | GPIO_Pin_1);
    
    /*也可以逐个点亮，指令运行很快，两种方式看不出区别*/
//  GPIO_ResetBits(GPIOB, GPIO_Pin_5);
//  GPIO_ResetBits(GPIOB, GPIO_Pin_0);
//  GPIO_ResetBits(GPIOB, GPIO_Pin_1);
  
    /*如果三个GPIO输出高电平，熄灭三个LED*/
//  GPIO_SetBits(GPIOB, GPIO_Pin_5 | GPIO_Pin_0 | GPIO_Pin_1);

    /*开启 GPIOA 的时钟，才能使用 GPIOA*/
    RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOA, ENABLE); //AHB1上开启GPIOA时钟
    
    /*配置蜂鸣器所在的GPIO（PA8）*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8; //要初始化的引脚号
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;  //GPIO设置为输出模式
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;  //GPIO设置为推挽模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;  //速度设置为25MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;  //没有上拉下拉电阻
    GPIO_SetBits(GPIOA, GPIO_Pin_8);//预先设置为高电平，蜂鸣器是不响的
    GPIO_Init( GPIOA,  &GPIO_InitStructure );  //使用上述的数据，初始化GPIOA

    /*PA8输出低电平，蜂鸣器响*/
//  GPIO_ResetBits(GPIOA, GPIO_Pin_8);

    /*如果PA8输出高电平，蜂鸣器停止鸣响*/
//  GPIO_SetBits(GPIOA, GPIO_Pin_8);

    /* [扩展实验：实现不同的LED、蜂鸣器亮灭组合] */
    GPIO_ResetBits(GPIOB, GPIO_Pin_5 | GPIO_Pin_1);
    GPIO_ResetBits(GPIOA, GPIO_Pin_8);
    //以上实现了LED1、LED3、蜂鸣器工作的组合。

    /*以上代码都是初始化代码，实验2-1还没有循环的代码，但是循环本身必须有，因此初始化后添加一个while(1)循环*/
    while(1){}
}
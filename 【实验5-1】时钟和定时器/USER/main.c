#include "stm32f4xx.h"

GPIO_InitTypeDef GPIO_InitStructure; //定义一个GPIO初始化用的结构体
TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure; //定义一个定时器初始化结构体
NVIC_InitTypeDef NVIC_InitStructure; //定义一个NVIC初始化结构体

int main(void){
    /*初始化LED1的GPIO，用于闪烁(PB5)*/
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);//使能LED1的IO时钟
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5; //选择LED PB5的IO
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT; //输出模式
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽输出模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; //LED可以慢一些省电
    GPIO_SetBits(GPIOB,GPIO_Pin_5); //预先设置LED1（PB5）为熄灭
    GPIO_Init(GPIOB, &GPIO_InitStructure); //初始化

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE); ///使能定时器TIM3时钟
    TIM_TimeBaseInitStructure.TIM_Period = 4999; //预装载值
    TIM_TimeBaseInitStructure.TIM_Prescaler = 8400; //定时器预分频
    TIM_TimeBaseInitStructure.TIM_CounterMode= TIM_CounterMode_Up;//向上计数模式
    TIM_TimeBaseInit(TIM3,&TIM_TimeBaseInitStructure);//初始化定时器TIM3

    TIM_ClearFlag(TIM3, TIM_FLAG_Update); //清除一次定时器更新标志

    TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE); //开启定时器3外设中断
    NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn; //定时器3中断向量
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x01; //抢占优先级1
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x03; //子优先级3
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    TIM_Cmd(TIM3,ENABLE); //使能定时器3,计时开始

    while(1)
    {
    //主循环中没有内容
    }

}

/*定时器3中断服务函数*/
void TIM3_IRQHandler(void){
    static u8 i; //定义静态变量i，每次用完不会被清零
    if(TIM_GetITStatus(TIM3,TIM_IT_Update)==SET){ //判断是定时器3溢出中断
        if(i){ //用户代码：根据i的值点亮或熄灭LED1(PB5)
            i = 0;
            GPIO_ResetBits(GPIOB,GPIO_Pin_5);//点亮LED1(PB5)
        }else{
            i = 1;
            GPIO_SetBits(GPIOB,GPIO_Pin_5);//熄灭LED1(PB5)
        }
    }
    TIM_ClearITPendingBit(TIM3,TIM_IT_Update);  //清除中断标志位
}
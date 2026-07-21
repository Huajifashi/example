#include "stm32f4xx.h"

u16 PWM_Value = 0; //PWM值的临时变量
u16 Adc = 0;  //ADC值  

/*定义所需的初始化结构体变量*/
GPIO_InitTypeDef   GPIO_InitStructure;
TIM_TimeBaseInitTypeDef    TIM_TimeBaseStructure;
TIM_OCInitTypeDef    TIM_OCInitStructure;

/* 函数声明 */
void delay_us(uint16_t nus);//纳秒的延时函数声明
void delay_ms(u16 nms);//毫秒的延时函数声明
void Adc_Init(void);     //ADC初始化函数声明
uint16_t  Get_Adc(uint8_t ch);     //ADC转换一次的函数声明        
uint16_t  Get_Adc_Average(uint8_t ch,uint8_t times);//ADC转换多次的函数声明

int main(void){
    //此部分参照之前代码
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);  //使能TIM3时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE); //使能PORTB时钟
    
    GPIO_PinAFConfig(GPIOB,GPIO_PinSource5,GPIO_AF_TIM3); //PB5为定时器3复用
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;   //PB5
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;  //IO复用功能
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//速度100MHz
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;  //推挽复用输出
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;  //上拉
    GPIO_Init(GPIOB,&GPIO_InitStructure);  //初始化PB5
      
    TIM_TimeBaseStructure.TIM_Prescaler = 84;      //定时器分频
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //向上计数模式
    TIM_TimeBaseStructure.TIM_Period = 500-1;          //自动重装载值
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; 
    
    TIM_TimeBaseInit(TIM3,&TIM_TimeBaseStructure);//初始化TIM3
     
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;   //脉宽调制模式
     TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //使能比较输出
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low; //输出极性
    TIM_OC2Init(TIM3, &TIM_OCInitStructure);  

    TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable); //使能TIM3在CCR1上的预装载寄存器
 
      TIM_ARRPreloadConfig(TIM3,ENABLE);   //使能ARPE 
    
    TIM_Cmd(TIM3, ENABLE);    //使能TIM3

    Adc_Init();

    /*间隔0.5秒，依次输入四个PWM值，LED1亮度会产生四种变化最后熄灭*/
    TIM_SetCompare2(TIM3,499);  //写入PWM值(0--499)
    delay_ms(500);
    TIM_SetCompare2(TIM3,300);  //写入PWM值(0--499)
    delay_ms(500);    
    TIM_SetCompare2(TIM3,100);  //写入PWM值(0--499)
    delay_ms(500);    
    TIM_SetCompare2(TIM3,0);  //写入PWM值(0--499)

    while(1)
    {
        Adc = Get_Adc_Average(ADC_Channel_11,10);//获取ADC10次平均值
        PWM_Value = 455-Adc/9;
        TIM_SetCompare2(TIM3,PWM_Value);  //写入PWM值
        
        delay_ms(100);//检测间隔20毫秒，每秒50次（0-499大约10秒结束）
    }
}

void  Adc_Init(void)
{    
GPIO_InitTypeDef      GPIO_InitStructure;   //定义GPIO初始化结构体
    ADC_CommonInitTypeDef ADC_CommonInitStructure; //定义ADCCommon结构体
    ADC_InitTypeDef       ADC_InitStructure;    //定义ADC初始化结构体
    
     RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);//使能AHB1上的GPIOA时钟
RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE); //使能APB2上的ADC1时钟

GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;  //PC 1
GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN; //配置为模拟模式
GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ; //没有上下拉电阻
GPIO_Init(GPIOC, &GPIO_InitStructure);  //初始化PC1
 
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1,ENABLE);      //ADC复位
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1,DISABLE);    //ADC复位结束
 
    
ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;   //ADC设置为独立模式
ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;   //2采样之间5个时钟
ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;  //不使用DMA
ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div6;  //预分频=6 PCLK2/6 = 84/6 = 14Mhz
ADC_CommonInit(&ADC_CommonInitStructure);
    
ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;   //12bit模式
ADC_InitStructure.ADC_ScanConvMode = DISABLE;            //非扫描模式    
ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;      //关闭连续转换
ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None; 
ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;   //右对齐    
ADC_InitStructure.ADC_NbrOfConversion = 1;               //转换序列1 
ADC_Init(ADC1, &ADC_InitStructure);
    
    ADC_Cmd(ADC1, ENABLE);  //ADC开启
}

uint16_t Get_Adc(u8 ch)   //ADC转换一次，480周期
{    
/*  配置ADC转换参数  */
ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_480Cycles );                

ADC_SoftwareStartConv(ADC1); //开启ADC转换
     
while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC )); //等待ADC转换完成

return ADC_GetConversionValue(ADC1);    //返回ADC转换数据
}

/*  应当转换多次数据取平均值才精确 */
/*  转换次数times一般不少于8次  */
uint16_t Get_Adc_Average(uint8_t ch,uint8_t times) 
{
    uint32_t temp_val=0;//定义一个32位变量用于累加多次的16位数据
    uint8_t  i;//循环变量
    for(i = 0;i < times; i++)//循环times次数，每次转换一个ADC数据
    {
        temp_val+=Get_Adc(ch);//转换（采集）ADC数据
        delay_ms(5); //延时一小会
    }
    return temp_val/times;//返回平均值：累加值 / 次数
}

/*****  简单延时函数,ms,us  *****/
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
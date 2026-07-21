#include <stdio.h>
#include "stm32f4xx.h"
#include "delay.h"     //简单延时

#define IR_RECV_TIM   TIM9

//遥控器接收状态
//Bit[7]:收到了引导码标志
//Bit[6]:得到了一个按键的所有信息
//Bit[5]:保留
//Bit[4]:标记是否收到第一个上升沿
uint8_t IRState  = 0;
#define IR_STATE_LEAD_OK  0x80   //标记是否已接收到前导码
#define IR_STATE_DATA_OK  0x40   //标记此键值是否已接收完
#define IR_STATE_UP_OK    0x10   //标记是否收到第一个上升沿

uint32_t  CapValue = 0;  //捕获信号时的时长
uint32_t  IRCount  = 0; //按键按下的次数
uint32_t  IRData   = 0;  //红外接收到的完整数据

void IR_GPIO_Init(void);
void IR_TIM_Init(void);

void IR_Recv_Init(){
    IR_GPIO_Init();
    IR_TIM_Init();
}

void IR_GPIO_Init(void){
    GPIO_InitTypeDef GPIO_InitStruct;
     RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE); //使能时钟 

    //PE5
    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_5; 
     GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF;
     GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_PuPd  = GPIO_PuPd_UP;
     GPIO_Init(GPIOE, &GPIO_InitStruct);

    GPIO_PinAFConfig(GPIOE, GPIO_PinSource5, GPIO_AF_TIM9);
}

void IR_TIM_Init(void){
    NVIC_InitTypeDef NVIC_InitStruct;
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStruct;
    TIM_ICInitTypeDef  TIM_ICInitStruct;  
 
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM9,ENABLE);    //TIM时钟使能 

     TIM_TimeBaseStruct.TIM_Period    = 10000; //自动装载值, 周期10ms
    TIM_TimeBaseStruct.TIM_Prescaler = 167;//预分频器,1M的计数频率,1us加1     
    TIM_TimeBaseStruct.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割
    TIM_TimeBaseStruct.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
    TIM_TimeBaseInit(IR_RECV_TIM, &TIM_TimeBaseStruct);

    TIM_ICInitStruct.TIM_Channel     = TIM_Channel_1; 
    TIM_ICInitStruct.TIM_ICPolarity  = TIM_ICPolarity_Rising;
    TIM_ICInitStruct.TIM_ICSelection = TIM_ICSelection_DirectTI;
    TIM_ICInitStruct.TIM_ICPrescaler = TIM_ICPSC_DIV1;     //配置输入分频,不分频 
    TIM_ICInitStruct.TIM_ICFilter    = 0x03; //IC1F=0003 8个定时器时钟周期滤波
    TIM_ICInit(IR_RECV_TIM, &TIM_ICInitStruct);        //初始化定时器输入捕获通道
    
    TIM_ITConfig( TIM9, TIM_IT_Update | TIM_IT_CC1, ENABLE);//允许更新中断
    TIM_Cmd(IR_RECV_TIM, ENABLE ); //使能定时器
        
    NVIC_InitStruct.NVIC_IRQChannel = TIM1_BRK_TIM9_IRQn;  //TIM中断
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;  //先占优先级1级
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 3;  //从优先级3级
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
    NVIC_Init(&NVIC_InitStruct);
}

//定时器输入捕获中断回调函数
void TIM1_BRK_TIM9_IRQHandler(){
    
    //一个IR信号位由560us的脉冲低电平+间隔高电平组成，，
    //只需捕获前后两个上升沿，之间即为完整一个IR信号位，减去560us脉冲时长，即为间电高平时长
    if(TIM_GetITStatus(IR_RECV_TIM, TIM_IT_CC1) != RESET){
        TIM_ClearITPendingBit(IR_RECV_TIM, TIM_IT_CC1);
        TIM_SetCounter(IR_RECV_TIM, 0); //清空定时器值
        
        if( !(IRState & IR_STATE_UP_OK) ){//第一次上升沿只表示一组新信号开始，后续开始计时    
            IRState |= IR_STATE_UP_OK; //标记
            return;
        }
        
        //两次上升沿的间隔时间-560us脉冲即为高电平时长
        CapValue = TIM_GetCapture1(IR_RECV_TIM) - 560; 
        
        //根据高电平时长做不同判断
        if(CapValue>300 && CapValue<800){//数据位0，标准值560us
            IRData >>= 1; //右移一位.NEC为LSB先发
            //printf("0");
        }
        else if(CapValue>1400 && CapValue<1800){ //数据位1，标准值为1680us
            IRData >>= 1; //右移一位.NEC为LSB先发
            IRData |=  0x80000000; //接收到1
            //printf("1");
        }
        else if(CapValue>2200 && CapValue<2600){ //得到按键键值增加的信息2500为标准值2.5ms
            IRCount++; //按键次数增加1次
        }
        else if(CapValue>4200 && CapValue<5000){  //4500为标准值4.5ms
            IRData  = 0 ;
            IRState |= IR_STATE_LEAD_OK;   //标记成功接收到了引导码
            IRCount = 0;  //清除按键次数计数器
            //printf("S");
        }
    }

    
    //Timer溢出中断
    //正常接收过程不会引起溢出，只有已完整收完，长期高电平时才可能溢出
    if(TIM_GetITStatus(IR_RECV_TIM, TIM_IT_Update) != RESET){//超时溢出中断
        TIM_ClearITPendingBit(IR_RECV_TIM, TIM_IT_Update);    
        
        if( IRState & IR_STATE_LEAD_OK ) {//前导码尚未清除，表示仍在此键码接收状态
            IRState &= ~IR_STATE_UP_OK;  //取消上升沿已经被捕获标记，即开始捕获新脉冲
            IRState &= ~IR_STATE_LEAD_OK;//清空引导标识，即开始新键码接收

            printf("IRRecv:[%X]\r\n", IRData);
            IRState |= IR_STATE_DATA_OK ;  //标记已经完成一次按键的键值信息采集
        }
    }
}

//实际键码处理函数，由应用程序可重写替换
__attribute__((weak)) void IR_Rece_Proc(uint16_t addr, uint8_t code ){
    printf("IRRecv:addr:%d, code:%d\r\n", addr, code);
}

//处理红外任务
void IR_Recv( void ) {
    uint16_t addr = 0; //地址码
    uint8_t byte1,byte2,byte3,byte4 = 0;

    if( !(IRState & IR_STATE_DATA_OK) ) {//数据接收OK
        return;
    }

    IRState &= ~IR_STATE_DATA_OK;//清除接收到有效按键标识

    byte1 = IRData;       //地址码
    byte2 = IRData >> 8;  //地址反码
    byte3 = IRData >> 16; //键码
    byte4 = IRData >> 24; //键码反码
    IRData = 0;

    //检验遥控键码
    if(byte3 != (uint8_t)~byte4){ return; }
    
    //两地址码若不是反码则为扩展协议，两字都为地址码
    if(byte1 != (uint8_t)~byte2){
        addr = IRData;//取低16位为地址码
    }
    else{
        addr = byte1;
    }

    IR_Rece_Proc(addr, byte3);
}


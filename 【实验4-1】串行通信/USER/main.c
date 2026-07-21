#include "stm32f4xx.h"

/* 变量定义区 */
u16 i = 0; //一个循环用变量
u8 OD_Flag = 0; //收到换行符的标志
u8 Rx_Frame_Flag = 0; //收到换行符的标志
u8 Rx_Buf[64]; //接收缓存
u16 Rx_Con; //接收计数器
u16 Rx_Len; //接收长度
u8 Tx_Buf[64]; //发送缓存
u8 Error = 0; //错误指令的标志

/* 结构体变量定义 */
GPIO_InitTypeDef GPIO_InitStructure; //定义一个GPIO初始化用的结构体
USART_InitTypeDef USART_InitStructure; //定义一个串口初始化用的结构体
NVIC_InitTypeDef NVIC_InitStructure; //定义一个NVIC初始化用的结构体  

/* 函数声明区 */
void USART2_Send_Frame(u8* data,u16 len);//声明串口的发送函数
void delay_us(uint16_t nus); //声明微秒（us）的延时函数
void delay_ms(u16 nms); //声明毫秒（ms）的延时函数

int main(void){
    /*配置三个LED*/
    /*开启 GPIOB 的时钟，才能使用 GPIOB*/
    RCC_AHB1PeriphClockCmd ( RCC_AHB1Periph_GPIOB, ENABLE );//AHB1上开启GPIOB时钟
    /*配置三个LED所在的GPIO（PB5、PB0、PB1）*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_0 | GPIO_Pin_1; //要初始化的引脚号（PB5、PB0、PB1）
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT; //GPIO设置为输出模式
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //GPIO设置为推挽模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz; //速度设置为25MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL; //没有上拉下拉电阻
    /*预先把输出设置为高电平，确保LED时初始化后是熄灭的*/
    GPIO_SetBits(GPIOB, GPIO_Pin_5 | GPIO_Pin_0 | GPIO_Pin_1);
    /*使用上面配置的GPIO_InitStructure数据，初始化GPIOB*/
    GPIO_Init( GPIOB, &GPIO_InitStructure );

    /*配置蜂鸣器*/
    /*开启 GPIOA 的时钟，才能使用 GPIOA*/
    RCC_AHB1PeriphClockCmd ( RCC_AHB1Periph_GPIOA, ENABLE ); //AHB1上开启GPIOA时钟

    /*配置蜂鸣器所在的GPIO（PA8）*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8; //要初始化的引脚号
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT; //GPIO设置为输出模式
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //GPIO设置为推挽模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz; //速度设置为25MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL; //没有上拉下拉电阻
    GPIO_SetBits(GPIOA, GPIO_Pin_8);//预先设置为高电平，蜂鸣器不响的
    GPIO_Init( GPIOA, &GPIO_InitStructure ); //使用上述的数据，初始化GPIOB

    /*使能GPIOA和USART2时钟*/
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);//GPIOA在AHB1高速总线上
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE);//USART2在APB1硬件总线上

    /*USART2 GPIO配置*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF; //复用模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//速度50MHz
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽复用输出
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //上拉电阻
    GPIO_Init(GPIOA,&GPIO_InitStructure);

    /*USART2 对应引脚复用映射*/
    GPIO_PinAFConfig(GPIOA,GPIO_PinSource2,GPIO_AF_USART2);//映射PA2到USART2上
    GPIO_PinAFConfig(GPIOA,GPIO_PinSource3,GPIO_AF_USART2);//映射PA3到USART2上

    /*USART2 端口配置*/
    USART_InitStructure.USART_BaudRate = 115200; //波特率设置
    USART_InitStructure.USART_WordLength = USART_WordLength_8b; //字长8bit
    USART_InitStructure.USART_StopBits = USART_StopBits_1; //1停止位
    USART_InitStructure.USART_Parity = USART_Parity_No; //无校验
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx; //收发
    USART_Init(USART2, &USART_InitStructure); //使用上面的参数初始化USART2

    USART_Cmd(USART2, ENABLE); //使能串口2
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE); //开中断

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0); //选择中断数量种类

    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn; //USART2中断向量
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; //抢占优先级1
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3; //子优先级3
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道使能
    NVIC_Init(&NVIC_InitStructure);

    /** 初始化串口后，发送这一帧数据出去 **/
    USART2_Send_Frame("Hello Everyone!\r\n", 17);

    while(1)
    {
        if(Rx_Frame_Flag)//USART2 收到一帧数据
        {
            Rx_Frame_Flag = 0;//清除标志位
            if(4 == Rx_Len)//数据长度是4（BEEP、LEDx）
            {
                if(('B' == Rx_Buf[0])&&('E' == Rx_Buf[1])&&('E' == Rx_Buf[2])&&('P' == Rx_Buf[3]))
                {
                    GPIO_ResetBits(GPIOA, GPIO_Pin_8);//蜂鸣器响
                    delay_ms(50);//延时一小会
                    GPIO_SetBits(GPIOA, GPIO_Pin_8);//蜂鸣器不响
                    USART2_Send_Frame("蜂鸣器\r\n", 8);//一个汉字两个字节，加回车共8字节
                }
                else if(('L' == Rx_Buf[0])&&('E' == Rx_Buf[1])&&('D' == Rx_Buf[2]))
                {
                    if('1' == Rx_Buf[3])//数据是LED1
                    {
                        GPIO_ResetBits(GPIOB, GPIO_Pin_5);//点亮LED1
                        USART2_Send_Frame("LED1\r\n", 6);
                    }
                    else if('2' == Rx_Buf[3])//数据是LED2
                    {
                        GPIO_ResetBits(GPIOB, GPIO_Pin_0);//点亮LED2
                        USART2_Send_Frame("LED2\r\n", 6);
                    }
                    else if('3' == Rx_Buf[3])//数据是LED3
                    {
                        GPIO_ResetBits(GPIOB, GPIO_Pin_1);//点亮LED3
                        USART2_Send_Frame("LED3\r\n", 6);
                    }else
                    {
                        Error = 1;//指令错误，需要报错
                    }

                    delay_ms(200);//无论点亮哪一个，延时200ms关闭
                    GPIO_SetBits(GPIOB, GPIO_Pin_5 | GPIO_Pin_0 | GPIO_Pin_1);
                }
                else
                {
                    Error = 1;//指令错误，需要报错
                }
            }
            else
            {
                Error = 1;//指令错误，需要报错
            }
            
            if(Error)//发送报错信息
            {
                Error = 0;
                USART2_Send_Frame("指令错误！\r\n", 12);//错误的指令
            }
        }
    }
}

/******** UART2 帧发送函数 ********/
/* u8* data：要发送的数据（数组）*/
/*u16 len：要发送数据的长度（字节数）*/
void USART2_Send_Frame(u8* data,u16 len)
{
    u16 i;
    for(i = 0;i < len;i++) //循环发送
    {
        while(USART_GetFlagStatus(USART2,USART_FLAG_TC)==RESET); //等待发送器空闲
        USART_SendData(USART2, data[i]);//发一个字节出去
    }
}

/***  串口2中断处理函数  ***/
void USART2_IRQHandler(void)
{
u8 res = 0;//临时变量
    
/****接收中断服务函数****/
if(USART_GetFlagStatus(USART2,USART_FLAG_RXNE))    
    {
        res = USART_ReceiveData(USART2); //读取接受到的数据
        
        /*根据是否收到回车符0x0D,区分不同情况*/
        if(OD_Flag)//已经接收到了0x0D回车符的情况
        {
            if(res == 0x0A) //又收到了0x0A换行符,代表这一帧数据结束
            {
                /* 把这一帧数据发出去 */
                Rx_Frame_Flag = 1;//置位标志
                Rx_Len = Rx_Con;//获取长度
                Rx_Con = 0;//清零计数
                OD_Flag = 0;//清零回车标志
            }
        }
        else //还没收到0x0D回车符的情况
        {
            if(res==0x0D) //收到回车符0x0D
            {
                OD_Flag = 1;//回车符标志置1
            }
            else
            {
                Rx_Buf[Rx_Con] = res;  //收到的正常数据，放进缓存中
                Rx_Con++; //缓存地址加1
            }
        }
    }
}

/***** 简单延时函数,ms,us *****/
void delay_us(uint16_t nus)
{
    uint16_t i;
    while(nus--)
    {
        i = 31; //秒表1分钟测试31
        while(i--);
    }
}

void delay_ms(u16 nms)
{
    uint16_t i;
    while(nms--)
    {
        i = 33800; //秒表1分钟测试33800
        while(i--);
    }
}
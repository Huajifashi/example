#include "stm32f4xx.h"
#include "delay.h"

/*定义一帧数据，准备发送，\r\n代表回车换行，共两个字节*/
u8 SendBuf[] = "Hello Everyone!\r\n"; //全长17个字节

void USART2_Init( u32 baudrate );

int main(void){
    DMA_InitTypeDef DMA_InitStruct;//定义 DMA 初始化结构体
    USART2_Init(115200); //串口2 初始化波特率115200

    USART_DMACmd(USART2,USART_DMAReq_Tx,ENABLE); //使能串口2的DMA发送
    USART_DMACmd(USART2,USART_DMAReq_Rx,ENABLE); //使能串口2的DMA接收
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1,ENABLE); //开启DMA1时钟

    /* 配置 DMA Stream */
    /*配置 DMA1 的通道、外设地址、存储器地址、数据方向、数量和递增要求*/
    DMA_InitStruct.DMA_Channel = DMA_Channel_4; //通道选择
    DMA_InitStruct.DMA_PeripheralBaseAddr = (u32)&USART2->DR;//DMA外设地址设为串口2
    DMA_InitStruct.DMA_Memory0BaseAddr = (u32)SendBuf;//DMA 存储器地址：要发送的地址
    DMA_InitStruct.DMA_DIR = DMA_DIR_MemoryToPeripheral;//存储器-->外设模式
    DMA_InitStruct.DMA_BufferSize = 17;//数据传输量
    DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;//外设地址不递增
    DMA_InitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;//存储器地址递增

    /* 配置 DMA1 其他参数 */
    DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;//外设数据长度:8位
    DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;//存储器数据长度:8位
    DMA_InitStruct.DMA_Mode = DMA_Mode_Normal;// 使用普通模式
    DMA_InitStruct.DMA_Priority = DMA_Priority_Medium;//中等优先级
    DMA_InitStruct.DMA_FIFOMode = DMA_FIFOMode_Disable; //不使用FIFO
    DMA_InitStruct.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
    DMA_InitStruct.DMA_MemoryBurst = DMA_MemoryBurst_Single;//存储器突发单次传输
    DMA_InitStruct.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;//外设突发单次传输

    DMA_Init(DMA1_Stream6, &DMA_InitStruct);//使用上述参数 初始化DMA Stream6
    DMA_Cmd(DMA1_Stream6, ENABLE); //最后，启动 DMA 发送数据到串口2，电脑端接收到数据

    while(1){}
}

//初始化串口2
void USART2_Init( u32 baudrate ){
    GPIO_InitTypeDef GPIO_InitStructure; //定义一个GPIO初始化用的结构体
    USART_InitTypeDef USART_InitStructure; //定义一个串口初始化用的结构体

    /*使能GPIOA和USART2时钟*/
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE); //GPIOA在AHB1高速总线上
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE); //USART2在APB1硬件总线上

    /*USART2 GPIO配置*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3; //USART2的IO
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF; //复用模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//速度50MHz
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽复用输出
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //上拉电阻
    GPIO_Init(GPIOA,&GPIO_InitStructure);

    /*USART2 对应引脚复用映射*/
    GPIO_PinAFConfig(GPIOA,GPIO_PinSource2,GPIO_AF_USART2); //映射PA2到USART2上使用
    GPIO_PinAFConfig(GPIOA,GPIO_PinSource3,GPIO_AF_USART2); //映射PA3到USART2上使用

    /*USART2 端口配置*/
    USART_InitStructure.USART_BaudRate = baudrate; //波特率设置
    USART_InitStructure.USART_WordLength = USART_WordLength_8b; //字长8bit
    USART_InitStructure.USART_StopBits = USART_StopBits_1; //1停止位
    USART_InitStructure.USART_Parity = USART_Parity_No; //无校验
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx; //收发
    USART_Init(USART2, &USART_InitStructure); //使用上面的参数初始化USART2

    USART_Cmd(USART2, ENABLE); //使能串口2
}
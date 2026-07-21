#include <stdio.h>
#include "usart.h"     

void UART2_Init(u32 bound)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    //使能GPIOA和USART1时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);  //GPIOA在AHB1高速总线上
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE); //USART2在APB1硬件总线上

    //USART2 对应引脚复用映射
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2); 
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2); 
    
    //USART2 IO配置
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_2 | GPIO_Pin_3; 
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;     //复用
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//速度50MHz
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;   //推挽复用输出
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;    
    GPIO_Init(GPIOA,&GPIO_InitStructure);  
    
    //USART2 端口配置
    USART_InitStructure.USART_BaudRate = bound;  //波特率设置
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;  //字长8bit
    USART_InitStructure.USART_StopBits   = USART_StopBits_1;       //1停止位
    USART_InitStructure.USART_Parity     = USART_Parity_No;          //无校验
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;    //收发
    USART_Init(USART2, &USART_InitStructure); 
    
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;         //USART2中断向量
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3; //抢占优先级3
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;          //子优先级3
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;              //IRQ通道使能
    NVIC_Init( &NVIC_InitStructure );    
    
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//开中断
    USART_Cmd(USART2, ENABLE);  //使能串口2 
}


/*
发送一个字符 
*/
void UART_SendByte( USART_TypeDef * pUSARTx, uint8_t ch)
{
    //发送一个字节数据到USART
    USART_SendData(pUSARTx, ch);
        
    //等待发送数据寄存器为空
    while (USART_GetFlagStatus(pUSARTx, USART_FLAG_TXE) == RESET);    
}

/*
发送字符串
*/
void UART_SendString( USART_TypeDef * pUSARTx, char *str)
{
    unsigned int k=0;
    do {
      UART_SendByte( pUSARTx, *(str + k) );
      k++;
    } while(*(str + k)!='\0');
  
  //等待发送完成
  while(USART_GetFlagStatus(pUSARTx,USART_FLAG_TC)==RESET){}
}

/*****************  发送一个16位数 **********************/
void UART_SendHalfWord( USART_TypeDef * pUSARTx, uint16_t ch)
{
    uint8_t temp_h, temp_l;    
    temp_h = (ch&0XFF00)>>8;      /* 取出高八位 */
    temp_l = ch&0XFF;      /* 取出低八位 */
    
    USART_SendData(pUSARTx,temp_h);         /* 发送高八位 */
    while (USART_GetFlagStatus(pUSARTx, USART_FLAG_TXE) == RESET);    
    
    USART_SendData(pUSARTx,temp_l);         /* 发送低八位 */
    while (USART_GetFlagStatus(pUSARTx, USART_FLAG_TXE) == RESET);    
}

//重定向c库函数printf到串口，重定向后可使用printf函数
int fputc(int ch, FILE *f)
{
    // 发送一个字节数据到串口
    USART_SendData(USART2, (uint8_t) ch);
        
    // 等待发送完毕
    while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);        

    return (ch);
}

//重定向c库函数scanf到串口，重写向后可使用scanf、getchar等函数
int fgetc(FILE *f){
    // 等待串口输入数据
    while (USART_GetFlagStatus(USART2, USART_FLAG_RXNE) == RESET);
    return (int)USART_ReceiveData(USART2);
}


/*
串口2中断处理函数  
*/
void USART2_IRQHandler( void ){
    u8 sbuf = 0;
  
    //接收中断服务函数
    if(USART_GetFlagStatus(USART2, USART_FLAG_RXNE)) {
        sbuf = USART_ReceiveData(USART2);//收到一个字节
        USART_SendData(USART2, sbuf);    //回显转发回去
    }
}
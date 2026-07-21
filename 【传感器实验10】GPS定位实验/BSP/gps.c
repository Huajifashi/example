#include "stm32f4xx.h"
#include "gps_parser.h"
#include "usart.h"

#define GPS_USART UART4

uint8_t g_GPS_RxBuf[500];
volatile uint16_t g_GPS_RxBufLen = 0;
volatile uint8_t g_GPS_RxDataOK = 0; //标志GPS当前帧是否接收完

/*
初始化GPS，主要为GPIO与UART接口
*/
void GPS_Init(uint32_t baud){
    GPIO_InitTypeDef GPIO_InitStructure; 
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;  

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC,  ENABLE);    //打开串口GPIO的时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE); //使能UART时钟
   
    //USART GPIO初始化
    //配置UART引脚为复用功能
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10|GPIO_Pin_11;  
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;  
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    GPIO_PinAFConfig(GPIOC, GPIO_PinSource10,  GPIO_AF_UART4);  //连接PXx到USARTx_Rx
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_UART4); //连接 PXx到USARTx_Tx

    //配置串口USART模式
    USART_InitStructure.USART_BaudRate   = baud;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;  //数据位8
    USART_InitStructure.USART_StopBits   = USART_StopBits_1;  //停止位：1个停止位
    USART_InitStructure.USART_Parity = USART_Parity_No;  //校验位选择：不使用校验
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;  //不使用硬件流
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;  //同时使能接收和发送
    USART_Init(GPS_USART, &USART_InitStructure);   //完成USART初始化配置

    USART_ITConfig(GPS_USART, USART_IT_RXNE, ENABLE);    //使能串口接收中断
    USART_ITConfig(GPS_USART, USART_IT_IDLE, ENABLE ); //使能串口总线空闲中断    
    USART_Cmd(GPS_USART, ENABLE);  // 使能串口
    
    //配置NVIC中断
    NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;          //配置USART为中断源
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;  //抢断优先级
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;           //优先级 
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;      //使能中断
    NVIC_Init(&NVIC_InitStructure);     //初始化配置NVIC    
}

void UART4_IRQHandler(void){
    //接收中断
    if(USART_GetITStatus(UART4, USART_IT_RXNE) != RESET){
        if(g_GPS_RxDataOK == 1){return;}//上批数据未处理，则不接收新数据
        if(g_GPS_RxBufLen >= 500){return;}

        g_GPS_RxBuf[g_GPS_RxBufLen++] = USART_ReceiveData( UART4 );
        USART_ClearITPendingBit(UART4, USART_IT_RXNE);
    }
    
    if(USART_GetITStatus(UART4, USART_IT_IDLE ) == SET ){  //数据帧接收完毕
        //通过先读SR，后读DR，以清除IDLE中断状态位
        UART4->SR; //先读SR寄存器
        UART4->DR; //再读DR寄存器
        g_GPS_RxDataOK = 1;
    }
}

void GPS_ReadAndParse(void){
    char *pGpsStart = NULL;
    char *pGpsEnd   = NULL;

#if 0  //For Test
    //char *gps_str = "$GNVTG,5.78,T,,M,0.00,N,0.00,K,D*2C\r\n";
    //char *gps_str = "$GPGSV,4,1,13,194,72,074,43,26,61,222,45,31,61,352,43,32,60,116,47,1*5A\r\n";
    //char *gps_str = "$GNGSA,A,3,22,29,26,25,03,32,31,16,194,193,,,1.35,0.74,1.13,1*0E\r\n";
    //char *gps_str = "$GNGLL,3149.333190,N,11706.911552,E,080237.000,A,D*42\r\n";
    //char *gps_str = "$GNRMC,080237.000,A,3149.333190,N,11706.911552,E,0.00,5.78,221121,,,D,V*06\r\n";
    char *gps_str = "$GNGGA,080237.000,3149.333190,N,11706.911552,E,2,15,0.74,53.489,M,-0.337,M,,*5F\r\n";
    gps_parse( gps_str );
#endif

    //读取一次串口数据
    while(g_GPS_RxDataOK != 1){} //等待接收完一批数据
    printf("================GPS DATA:\r\n");
    printf("%s", g_GPS_RxBuf);

    //依次解析每个语句
    pGpsStart = (char*)g_GPS_RxBuf;
    while( 1 ){
        pGpsStart = strpbrk(pGpsStart, "$");
        if(pGpsStart == NULL){//无GPS报文，跳出
            break;
        }

        //找结尾换行符，则查询
        pGpsEnd = strpbrk(pGpsStart, "\n");
        if(pGpsEnd == NULL){//无完整语句，跳出
            break;
        }

        gps_parse( pGpsStart );
        pGpsStart = pGpsEnd + 1;
    }

    //解析完，清除缓冲区，接收下一批数据
    memset(g_GPS_RxBuf, 0, sizeof(g_GPS_RxBuf));
    g_GPS_RxBufLen = 0;
    g_GPS_RxDataOK = 0;
}
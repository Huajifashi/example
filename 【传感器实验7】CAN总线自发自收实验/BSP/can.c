#include <stdio.h>
#include <string.h>
#include "stm32f4xx.h"

void CAN1_FilterConfig(void);

void CAN1_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    CAN_InitTypeDef  CAN_InitStruct={0};
    NVIC_InitTypeDef  NVIC_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//使能GPIO时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);//使能串口时钟
   
    //PA11、PA12复用为CAN1
    GPIO_InitStruct.GPIO_Pin  = GPIO_Pin_11 | GPIO_Pin_12;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    //引脚复用映射配置
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource11, GPIO_AF_CAN1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource12, GPIO_AF_CAN1);

    CAN_InitStruct.CAN_Prescaler = 6;       //分频系数(Fdiv)为brp+1    
    CAN_InitStruct.CAN_Mode = CAN_Mode_LoopBack;     //自发自收模式 
      CAN_InitStruct.CAN_SJW  = CAN_SJW_1tq;    //重新同步跳跃宽度(Tsjw)为tsjw+1个时间单位 CAN_SJW_1tq~CAN_SJW_4tq
      CAN_InitStruct.CAN_BS1  = CAN_BS1_6tq;  //Tbs1范围CAN_BS1_1tq ~CAN_BS1_16tq
      CAN_InitStruct.CAN_BS2  = CAN_BS2_7tq;  //Tbs2范围CAN_BS2_1tq ~    CAN_BS2_8tq
    CAN_InitStruct.CAN_TTCM = DISABLE;    //非时间触发通信模式   
      CAN_InitStruct.CAN_ABOM = DISABLE;    //软件自动离线管理      
      CAN_InitStruct.CAN_AWUM = DISABLE;  //睡眠模式通过软件唤醒(清除CAN->MCR的SLEEP位)
      CAN_InitStruct.CAN_NART = ENABLE;    //禁止报文自动传送 
      CAN_InitStruct.CAN_RFLM = DISABLE;    //报文不锁定,新的覆盖旧的  
      CAN_InitStruct.CAN_TXFP = DISABLE;    //优先级由报文标识符决定 
    CAN_Init(CAN1, &CAN_InitStruct);     // 初始化CAN1

    //中断配置
       CAN_ITConfig(CAN1, CAN_IT_FMP0, ENABLE);//FIFO0消息挂号中断允许.
      NVIC_InitStructure.NVIC_IRQChannel = CAN1_RX0_IRQn;
      NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;     // 主优先级为1
      NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;            // 次优先级为0
      NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
      NVIC_Init(&NVIC_InitStructure);

    CAN1_FilterConfig();
}

//接收筛选器配置
void CAN1_FilterConfig(void){
    CAN_FilterInitTypeDef  CAN_Filter;
    
    //配置过滤器
     CAN_Filter.CAN_FilterNumber     = 0;      //过滤器0
      CAN_Filter.CAN_FilterMode       = CAN_FilterMode_IdMask; 
      CAN_Filter.CAN_FilterScale      = CAN_FilterScale_32bit; //32位 
      CAN_Filter.CAN_FilterIdHigh     = 0x0000;//32位ID
      CAN_Filter.CAN_FilterIdLow      = 0x0000;
      CAN_Filter.CAN_FilterMaskIdHigh = 0x0000;//32位MASK
      CAN_Filter.CAN_FilterMaskIdLow  = 0x0000;
       CAN_Filter.CAN_FilterFIFOAssignment = CAN_Filter_FIFO0;//过滤器0关联到FIFO0
      CAN_Filter.CAN_FilterActivation     = ENABLE; //激活过滤器0
      CAN_FilterInit(&CAN_Filter);//滤波器初始化
}

//中断服务函数                
void CAN1_RX0_IRQHandler(void){
    int i = 0;
    uint32_t uCanID = 0;
      CanRxMsg RxMessage;
    
    CAN_Receive(CAN1, 0, &RxMessage);//接收数据

    uCanID = (RxMessage.IDE == CAN_Id_Standard) ? RxMessage.StdId : RxMessage.ExtId;
    printf("\nRecv CANID:0x%08X Data:[", uCanID);
    for(i=0; i<8; i++){
        printf("%02X ", RxMessage.Data[i]);
    }
    printf("]\r\n");
}

//发送消息
void CAN1_SendMsg(uint32_t uID, uint8_t *pData, uint32_t uLen){
    uint32_t i =0;
    uint8_t uMailBox = 0;    
    CanTxMsg TxMessage;

    TxMessage.StdId = uID;     // 标准标识符为0
    TxMessage.ExtId = uID;     // 设置扩展标示符（29位）
    TxMessage.IDE   = CAN_Id_Extended;    // 使用扩展标识符
    TxMessage.RTR   = CAN_RTR_Data;          // 消息类型为数据帧，一帧8位
    TxMessage.DLC   = uLen;             // 发送两帧信息
    
    printf("Send CAN Data:[0x%04X: ", uID);
    for(i=0; i<uLen; i++){
        printf("%02X ", pData[i]);
        TxMessage.Data[i] = pData[i];        // 第一帧信息          
    }
    printf("]\r\n");
    
    uMailBox = CAN_Transmit(CAN1, &TxMessage);       
    i = 0;
    while((CAN_TransmitStatus(CAN1, uMailBox)==CAN_TxStatus_Failed)) {//等待发送结束 
        i++;
        if(i>=0XFFF){ break; }
    }
    if(i>=0xFFF){
        printf("Send Failed!\r\n");
        return;
    }
    
    //printf("Send uccess! Tx_Mail:%u\r\n", uMailBox);
}
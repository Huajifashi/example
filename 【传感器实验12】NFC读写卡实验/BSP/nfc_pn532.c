#include <stdio.h>
#include <string.h>
#include "delay.h"
#include "nfc_pn532.h"

#define PN532_USART     USART1

#define LEN_ACK    6  //ACK帧的长度
#define INDEX_LEN  3  //LEN字节在命令中的偏移是
#define INDEX_TFI  5  //TFI字节在命令中的偏移是


#define CMD_SAMConfiguration    0x14
#define CMD_InListPassiveTarget 0x4A
#define CMD_InDataExchange      0x40

#define PN532_DELAY_MS(x) delay_ms(x)

//NFC读写等操作的尝试次数，0为永久尝试阻塞等待，>0为尝试次数，每次尝试失败等待100MS
//NFC所有操作都需扫描卡片，无卡时需等待放卡，此值用于设置尝试扫卡次数
uint32_t MAX_TRY = 0;

uint8_t UID[4]; //存储 UID
uint8_t UID_backup[4];//UID备份，用于处理不连续写同一卡

uint8_t KEY_A[6]={0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t KEY_B[6]={0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

uint8_t PN532_RxBuf[500];
uint16_t PN532_RxBufLen = 0;

int8_t PN532_WakeUp( void );//唤醒模块
int8_t PN532_InListPassiveTarget( void );   //配置
int8_t PN532_PsdVerifyKeyA( uint8_t *pKeyA );         //密码授权，验证KeyA
int8_t PN532_Read( uint8_t block, uint8_t* buf); //读指定块的16字节
int8_t PN532_Write(uint8_t block, uint8_t* buf); //指定块写16字节
int8_t PN532_SAMConfiguration( uint8_t mode );

void NFC_Init(uint32_t baud ){
    GPIO_InitTypeDef GPIO_InitStructure; 
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;  

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,  ENABLE);    //打开串口GPIO的时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);   //使能USART时钟
   
    //USART GPIO初始化
    //配置UART引脚为复用功能
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9|GPIO_Pin_10;  
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;  
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9,  GPIO_AF_USART1);  //连接PXx到USARTx_Rx
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1); //连接 PXx到USARTx_Tx


    //配置串口USART模式
    USART_InitStructure.USART_BaudRate   = baud;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;  //数据位8
    USART_InitStructure.USART_StopBits   = USART_StopBits_1;  //停止位：1个停止位
    USART_InitStructure.USART_Parity = USART_Parity_No;  //校验位选择：不使用校验
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;  //不使用硬件流
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;  //同时使能接收和发送
    USART_Init(PN532_USART, &USART_InitStructure);   //完成USART初始化配置

    USART_ITConfig(PN532_USART, USART_IT_RXNE, ENABLE);    //使能串口接收中断
    USART_Cmd(PN532_USART, ENABLE);  // 使能串口
    
    //配置NVIC中断
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;          //配置USART为中断源
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;  //抢断优先级
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;           //优先级 
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;      //使能中断
    NVIC_Init(&NVIC_InitStructure);     //初始化配置NVIC
}

//发送数据data
void PN532_SendData(uint8_t *data, uint8_t length){
    uint8_t i = 0;
    for(i=0; i<length; i++){
        USART_SendData(PN532_USART, data[i]);
        while(USART_GetFlagStatus(PN532_USART, USART_FLAG_TXE) == RESET){} //等得发送完成 
    }
}

/*
UART1中断处理函数
*/
void USART1_IRQHandler(void)
{
    //接收中断
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET){    
        PN532_RxBuf[PN532_RxBufLen++] = USART_ReceiveData( USART1 ); 
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}

/*
检验返回的数据是否合规
*/
int8_t PN532_CheckRxBuf(void){
    uint8_t i   = 0;
    uint8_t len = 0;
    uint8_t temp = 0;
        
    if(PN532_RxBufLen < (LEN_ACK + INDEX_TFI + 4)){ return -1;};//数据长度不够
    
    len = PN532_RxBuf[LEN_ACK + INDEX_LEN];//帧长
    temp = 0x100 - len;//计算LCS值
    if(temp != PN532_RxBuf[LEN_ACK + INDEX_LEN + 1]){return -1;}  //LCS不匹配
    
    //计算校验码
    temp = 0;
    for(i=0; i<len; i++){
        temp += PN532_RxBuf[LEN_ACK + INDEX_TFI + i];
    }
    temp = 0x100 - temp;
    if(temp != PN532_RxBuf[LEN_ACK + INDEX_TFI + len] ){ return -1;}//校检码不匹配

    return 0;
}

/*
计算给定数据的校验和
*/
uint8_t PN532_CheckSum(uint8_t *data){
    uint8_t i = 0;
    uint8_t temp = 0;
     
    for(i=0; i<data[INDEX_LEN]; i++){//计算校验和
        temp += data[INDEX_TFI + i];
    }
     
    return 0x100- temp;
}

/*
功能：唤醒NFC模块
可通过MAX_TRY设置尝试次数
唤醒数据:55 55 00 00 00 00 00 00 00 00 00 00 00 00 
返回值:
返回-1唤醒失败，返回0唤醒成功
*/
int8_t NFC_WakeUp(void){
    uint32_t i = 0;
    uint8_t data[14] = {0};
    data[0]  = 0x55;
    data[1]  = 0x55;
    
    while(1){
        PN532_SendData(data, sizeof(data)); //发送唤醒数据         
        if(PN532_SAMConfiguration(1) == 0){//使用SAM配置为普通模式
            printf("NFC WakeUP OK\r\n");
            break;
        }

        //稍作延时，再次尝试
        PN532_DELAY_MS(100);
        i++;
        if(MAX_TRY != 0 && i>MAX_TRY){
            printf("NFC WakeUP TimeOut\r\n");
            return -1;
        }
    }

    return 0;
}

void PN532_ClearRxBuf(void){
    PN532_RxBufLen = 0;
    memset(PN532_RxBuf, 0, sizeof(PN532_RxBuf));
}

/*
SAM配置
参数:
mode:
1: 普通模式, 
2: 虚拟卡模式
3: 有线卡模式
4: 双卡模式
*/
int8_t PN532_SAMConfiguration( uint8_t mode ){
    uint8_t data[10] = {0};
    int8_t ret = 0;
    
    data[0] = 0x00;  //前导码 Preamble
    data[1] = 0x00;  //开始码
    data[2] = 0xFF;  
    data[3] = 0x03; //包长度LEN
    data[4] = 0x100 - data[3]; //包长度校验LCS
    data[5] = 0xD4; //命令标识码 TFI
    data[6] = CMD_SAMConfiguration; //命令码
    data[7] = mode;  //配置为普通模式
    data[8] = PN532_CheckSum(data); //校验码
    data[9] = 0x00;  //后序字节 Postamble

    PN532_SendData(data, sizeof(data));
    PN532_DELAY_MS( 200 );

    if(PN532_CheckRxBuf()<0){ ret = -1; goto _END; }//返回数据格式不对
    ret = 0;

_END:
    PN532_ClearRxBuf();
    return ret;
}

/*
寻卡，扫描卡Mifare1卡
*/
int8_t PN532_InListPassiveTarget( void ){
    uint8_t data[11] = {0};
    int8_t ret = 0;

    data[0]  = 0x00; //前导码 Preamble
    data[1]  = 0x00; //开始码 
    data[2]  = 0xFF;
    data[3]  = 0x04; //包长度LEN
    data[4]  = 0x100 - data[3]; //包长度校验LCS
    data[5]  = 0xD4; //命令标识码 TFI
    data[6]  = CMD_InListPassiveTarget; //命令
    data[7]  = 0x01;  //只选1张卡
    data[8]  = 0x00;  //106K TypeA类型卡
    data[9]  = PN532_CheckSum(data); //数据校验
    data[10] = 0x00;  //后序码
    
    PN532_SendData(data, sizeof(data));
    PN532_DELAY_MS(200); 

    if(PN532_CheckRxBuf()<0){ ret = -1; goto _END; }//返回数据格式不对
    
    //卡数量，若Tg!=1则没检到卡
    if(PN532_RxBuf[LEN_ACK + INDEX_TFI + 2] != 1){ret = 0; goto _END; }

    //检到卡，保存扫描到的卡号UID
    UID[0] = PN532_RxBuf[LEN_ACK + INDEX_TFI + 8];
    UID[1] = PN532_RxBuf[LEN_ACK + INDEX_TFI + 9];
    UID[2] = PN532_RxBuf[LEN_ACK + INDEX_TFI + 10];
    UID[3] = PN532_RxBuf[LEN_ACK + INDEX_TFI + 11];
    ret = 1;
    
_END:
    PN532_ClearRxBuf();
    return ret;
}

/*
密码授权，验证KeyA
*/
int8_t PN532_PsdVerifyKeyA(uint8_t *pKeyA){
    uint8_t data[22] = {0};
    int8_t ret = 0;
    
    data[0] = 0x00;  //前导码
    data[1] = 0x00;  //开始码
    data[2] = 0xFF;  
    data[3] = 0x0F;  //包长度LEN
    data[4] = 0x100 - data[3]; //包长度校验
    data[5] = 0xD4; //命令标识码
    data[6] = CMD_InDataExchange; //命令码
    data[7] = 0x01;  //1号卡
    data[8] = 0x60;  //验证KEYA 
    data[9] = 0x03;  //
    
    data[10] = pKeyA[0]; //KEY A 密码 FF FF FF FF FF FF
    data[11] = pKeyA[1];
    data[12] = pKeyA[2];
    data[13] = pKeyA[3];
    data[14] = pKeyA[4];
    data[15] = pKeyA[5];
    
    data[16] = UID[0]; //指定卡UID
    data[17] = UID[1];
    data[18] = UID[2];
    data[19] = UID[3];
    data[20] = PN532_CheckSum(data);   //校验码
    data[21] = 0x00;  //后序码
    
    PN532_SendData(data, sizeof(data));
    PN532_DELAY_MS(200);

    if(PN532_CheckRxBuf()<0){ret = -1;goto _END;}//返回数据不正确    
    if(PN532_RxBuf[LEN_ACK + INDEX_TFI + 2] != 0){ ret=  -1; goto _END;}//status状态位不正确

    ret = 0;
_END:
    PN532_ClearRxBuf();
    return ret;
}

/*
写入指定块的数据
*/
int8_t PN532_Write(uint8_t block, uint8_t* buf){
    uint8_t data[28] = {0};
    int8_t ret = 0;

    if(buf == NULL){ return -1; }
    
    data[0] = 0x00; //前导码
    data[1] = 0x00; //开始码
    data[2] = 0xFF;
    data[3] = 0x15; //包长度LEN
    data[4] = 0x100 - data[3]; //包长度校验
    data[5] = 0xD4; //帧方向码
    data[6] = CMD_InDataExchange; //命令码
    data[7] = 0x01; //1号卡
    data[8] = 0xA0; //写16字节
    data[9] = block; //块地址     
    memcpy(data+10, buf, 16);
    data[26] = PN532_CheckSum(data); //数据校验码
    data[27] = 0x00;

    PN532_SendData(data, sizeof(data));
    PN532_DELAY_MS(200);

    if(PN532_CheckRxBuf()<0){ ret = -1; goto _END; }//返回数据不正确
    if(PN532_RxBuf[LEN_ACK + INDEX_TFI + 2] != 0){ ret = -1; goto _END;}//status状态位不正确
    ret = 0;

_END:
    PN532_ClearRxBuf();
    return ret;
}

/*
读指定块的16字节数据
*/
int8_t PN532_Read(uint8_t block , uint8_t *buf) {
    uint8_t data[12] = {0};
    int8_t ret = 0;

    if(buf == NULL){ return -1; }

    data[0] = 0x00;  //前导码
    data[1] = 0x00;  //开始码
    data[2] = 0xFF;    
    data[3] = 0x05; //包长度
    data[4] = 0x100 - data[3]; //包长度校验  
    data[5] = 0xD4; //命令标识码
    data[6] = 0x40; //命令码    
    data[7] = 0x01; //1号卡
    data[8] = 0x30; //读16字节命令
    data[9] = block; //指定地址
    data[10] = PN532_CheckSum(data); //校验码
    data[11] = 0x00; //后序码

    PN532_SendData(data, sizeof(data));
    PN532_DELAY_MS(200);

    if(PN532_CheckRxBuf()<0){ret = -1; goto _END;}//返回数据不正确
    if(PN532_RxBuf[LEN_ACK + INDEX_TFI + 2] != 0){ ret = -1; goto _END;}//status状态位不正确

    memcpy(buf, PN532_RxBuf + LEN_ACK + INDEX_TFI + 3, 16); //读数据
    ret = 0;

_END:
    PN532_ClearRxBuf();
    return ret;
}

/*
写入指定数据到指定块
返回-1写入失败，返回0写入成功
*/
int8_t NFC_Write(uint8_t block, uint8_t* buf){
    uint32_t i = 0;

    while(1){
        PN532_DELAY_MS(100);
        i++;
        if(MAX_TRY!=0 && i>MAX_TRY){//判断是否超过尝试次数
            printf("NFC Write TimeOut\r\n");
            return -1;
        }
        
        if(PN532_InListPassiveTarget()<=0){//扫描卡
            continue;
        }
        if(PN532_PsdVerifyKeyA(KEY_A) < 0){//验证密码A
            continue;
        }
        if(PN532_Write(block , buf)<0){//写数据
            continue;
        }
        break; //成功则跳出
    }
    
    return 0;
}

/*
读取指定数据
可通过MAX_TRY设置尝试次数
*/
int8_t NFC_Read(uint8_t block, uint8_t* buf){
    uint32_t i = 0;

    while(1){
        PN532_DELAY_MS(100);
        i++;
        if(MAX_TRY != 0 && i>MAX_TRY){//是否超过尝试次数
            printf("NFC Read TimeOut\r\n");
            return -1;
        }
        
        if(PN532_InListPassiveTarget() <= 0){//扫描卡
            continue;
        }
        if(PN532_PsdVerifyKeyA(KEY_A) < 0){//验证密码A
            continue;
        }
        if(PN532_Read(block , buf)<0){//读取块数据
            continue;
        }
        
        break; //成功则跳出
    }
    
    return 0;
}
#include <string.h>
#include "fpr_zn632.h"
#include "delay.h"

#define CMD_GetImage 0x01
#define CMD_GetEnrollImage 0x29
#define CMD_GenChar 0x02
#define CMD_Match 0x03
#define CMD_Search 0x04　
#define CMD_RegModel 0x05
#define CMD_StoreChar 0x06
#define CMD_LoadChar 0x07
#define CMD_UpChar 　　 0x08
#define CMD_DownChar 　　 0x09
#define CMD_UpImage 　　 0x0A
#define CMD_DownImage 　 0x0B
#define CMD_DeletChar 0x0C
#define CMD_Empty 0x0D
#define CMD_WriteReg 0x0E
#define CMD_ReadSysPara 0x0F
#define CMD_VryPwd 0x13
#define CMD_GetRandomCode 0x14
#define CMD_SetChipAddr 0x15
#define CMD_ReadINFpage 0x16
#define CMD_Port_Control 0x17
#define CMD_HighSpeedSearch 0x1B
#define CMD_ReadIndexTable 0x1F

#define ZN632_DELAY_MS(x) delay_ms(x) //延时

#define ZN632_USART USART1

#define ZN632_BUF_LEN 128 //接收缓冲区大小
uint32_t ZN632_RxBufLen = 0; //接收数据长度
uint8_t ZN632_RxBuf[ZN632_BUF_LEN] ; //接收缓冲区
uint8_t g_uExitToChange = 0; //强制退出当前任务，以便进行其他任务

uint8_t ZN632_ADDR[4] = {0xFF, 0xFF, 0xFF, 0xFF};

#define ZN632_INDEX_MAX 240 //指纹模块最大存储指纹模板数
uint8_t ZN632_INDEX[32] = {0}; //指纹ID索引值，每一位代表当前索引是否已录入

void ZN632_GPIO_Init(void);
void ZN632_UART_Init(uint32_t baud);
void ZN632_SendData(uint8_t *data, uint8_t length);
void ZN632_SendHead(void);
void ZN632_ShowError( uint16_t code);
void ZN632_ClearRxBuf(void);

/**
* @brief  初始化配置
* @param  无
* @retval 无
*/
void FPR_Init( uint32_t baud ){
    ZN632_GPIO_Init();
    ZN632_UART_Init(baud);
    ZN632_PowerOn();
}

//GPIO初始化
void ZN632_GPIO_Init(void){
    GPIO_InitTypeDef GPIO_InitStructure; 

    //电源GPC9 FPR_PWR线电源线
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);    
    
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;  
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}

//串口初始化
void ZN632_UART_Init(uint32_t baud){
    GPIO_InitTypeDef GPIO_InitStructure; 
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;  

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//打开串口外设的时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);//使能串口时钟

    //GPIO初始化
    //配置GPA9-Tx GPA10-Rx复用为UART1    
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_9 |GPIO_Pin_10;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;  
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9,  GPIO_AF_USART1); 
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);
    
    //配置串ZN632_USART工作参数
    USART_InitStructure.USART_BaudRate = baud;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b; //8数据位
    USART_InitStructure.USART_StopBits = USART_StopBits_1;  //1停止位
    USART_InitStructure.USART_Parity = USART_Parity_No;  //无校验位
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;  //无硬件流控制
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;  //收发双向
    USART_Init(ZN632_USART, &USART_InitStructure);

    USART_ITConfig(ZN632_USART, USART_IT_RXNE, ENABLE);     //使能接收中断
    USART_Cmd(ZN632_USART, ENABLE);

    //UART配置NVIC中断
    //NVIC_PriorityGroupConfig( NVIC_PriorityGroup_2 ); 
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;        //配置USART为中断源
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;    //抢断优先级
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;   //子优先级
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;      //使能中断
    NVIC_Init(&NVIC_InitStructure);     //初始化配置NVIC
}

/*
上电启动
*/
int16_t ZN632_PowerOn(void){
    uint32_t i = 0;
    GPIO_ResetBits(GPIOC, GPIO_Pin_9); //拉低电源控制线加电
    
    //等待成功字符
    while(1){     
        if(ZN632_RxBuf[0] == 0x55){//启动成功标志
            ZN632_ClearRxBuf();
            return 0;
        }
        
        i++;
        if(i>1000){ return -1; }//超时退出，未收到启动字符
        ZN632_DELAY_MS(1);
    }
}

/**
  * @brief  ZN632_USART串口向指纹模块传递数据 
  * @param  data;传输的数据
  * @param  data;数据长度
  */
void ZN632_SendData(uint8_t *data, uint8_t length){
    uint8_t i = 0;
    for(i=0; i<length; i++){
        USART_SendData(ZN632_USART, data[i]);
        while(USART_GetFlagStatus(ZN632_USART, USART_FLAG_TXE) == RESET){} //等待发送完成 
    }
}

/*
发送命令头
*/
void ZN632_SendHead( void ){
    uint8_t head[6] = {0};

    head[0] = 0xEF;  //包标识
    head[1] = 0x01;
    head[2] = ZN632_ADDR[0]; //地址
    head[3] = ZN632_ADDR[1];
    head[4] = ZN632_ADDR[2];
    head[5] = ZN632_ADDR[3];
    
    ZN632_SendData(head, 6);
}

/**
* @brief  串口中断服务函数,把接收到的数据写入缓冲区，
                    在main函数中轮询缓冲区输出数据
* @param  None
* @retval None
*/
void USART1_IRQHandler( void ){
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET){
        ZN632_RxBuf[ZN632_RxBufLen++] = USART_ReceiveData( USART1 );
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}

/*
串口清空接收缓冲区
*/
void ZN632_ClearRxBuf(void){
    memset(ZN632_RxBuf, 0, ZN632_RxBufLen);
    ZN632_RxBufLen = 0;
}

/*
检查返回ACK包是否合规  
*/
int16_t ZN632_CheckAck(){
    uint16_t i = 0;
    uint16_t temp = 0;
    uint16_t sum = 0;
    uint16_t len = 0;
        
    //包头检查
    if(ZN632_RxBuf[0] != 0xEF|| ZN632_RxBuf[1] != 0x01){ return -1; }
    
    //地址码检查
    if(ZN632_RxBuf[2] != ZN632_ADDR[0] || ZN632_RxBuf[3] != ZN632_ADDR[1] || 
       ZN632_RxBuf[4] != ZN632_ADDR[2] || ZN632_RxBuf[5] != ZN632_ADDR[3]){
        return -1;
    }
    
    len = ZN632_RxBuf[7] << 8 | ZN632_RxBuf[8];
    temp = 0;
    for(i=0; i<=len; i++){
        temp += ZN632_RxBuf[6+i];
    }
    sum = ZN632_RxBuf[9+len-2]<<8 | ZN632_RxBuf[9+len-1];//最后两字节为校验和
    if( sum != temp ){ return -1; } //校验码不对 
    
    //返回确认码
    return ZN632_RxBuf[9];
}

int16_t ZN632_VryPwd( void ){
    uint16_t temp = 0;
    int16_t  ret = 0;
    uint8_t cmd[10] = {0};
 
    cmd[0] = 0x01;         //包标识
    cmd[1] = 0x00;         //包长度
    cmd[2] = 0x07;         //包长度
    cmd[3] = CMD_VryPwd; //命令码
    cmd[4] = 0; //密码
    cmd[5] = 0; //密码
    cmd[6] = 0; //密码
    cmd[7] = 0; //密码
    
    temp = cmd[0] + cmd[1] + cmd[2] + cmd[3] + cmd[4] + cmd[5] + cmd[6] + cmd[7];
    cmd[8] = temp >> 8;
    cmd[9] = temp;
    
    ZN632_SendHead();
    ZN632_SendData(cmd, 10);

    ZN632_DELAY_MS( 300 );//等待指纹识别模块处理数据
    
    ret = ZN632_CheckAck();
    if(ret != 0){//显示错误信息
        ZN632_ShowError(ret);
    }
    
    ZN632_ClearRxBuf();
    return ret;
}

/**
获取指纹索引列表
*/
int16_t ZN632_ReadIndexTable( void ){
    uint16_t temp = 0;
    int16_t  ret = 0;
    uint8_t  cmd[7] = {0};
 
    cmd[0] = 0x01;     //包标识
    cmd[1] = 0x00;     //包长度
    cmd[2] = 0x04;     //包长度
    cmd[3] = CMD_ReadIndexTable; //命令码
    cmd[4] = 0; //页码,0-3，ZN632只支持页0
    
    temp = cmd[0] + cmd[1] + cmd[2] + cmd[3] + cmd[4];
    cmd[5] = temp >> 8;
    cmd[6] = temp;
    
    ZN632_SendHead();
    ZN632_SendData(cmd, 7);

    ZN632_DELAY_MS( 300 );//等待指纹识别模块处理数据
    
    ret = ZN632_CheckAck();
    if(ret != 0){//显示错误信息
        ZN632_ShowError(ret);
        goto _END;
    }
 
    //保存索引数据
    memcpy(ZN632_INDEX, ZN632_RxBuf+10, 32);
 
_END:    
    ZN632_ClearRxBuf();
    return ret;
}

/*
获取当前最小的可用空索引值
*/
uint16_t ZN632_GetIndexEmpty(void){
    uint16_t index = 0;
    uint16_t i,j = 0;
    
    for(i = 0;i < sizeof(ZN632_INDEX); i++){ //循环每个字节
        for(j = 0; j < 8;j++){ //循环一个字节的8bit
            if(ZN632_INDEX[i] & (0x1<<j) ){//该位已占用
                index++;
            }
            else {//为空则置1
                return index;
            }
        }
    }

    return ZN632_INDEX_MAX;
}

/*
将模板编号对应的标志位置1，表示已录入
*/
void ZN632_SetIndex(uint16_t uIndex){    
    uint8_t uByte = uIndex / 8;
    uint8_t uBit  = uIndex % 8;

    if(uIndex >= ZN632_INDEX_MAX){return;}
    
    ZN632_INDEX[uByte] |= 0x1<< uBit;
}

/*
清除标识志
*/
void ZN632_UnsetIndex(uint16_t uIndex){    
    uint8_t uByte = uIndex / 8;
    uint8_t uBit  = uIndex % 8;

    if(uIndex >= ZN632_INDEX_MAX){return;}
    
    ZN632_INDEX[uByte] &= ~(0x1 << uBit);
}

/**
  * @brief   获取图像，探测到后录入指纹图像存于ImageBuffer
  * @param   无
  * @retval  确认码=00H 表示录入成功；
             确认码=01H 表示收包有错；
             确认码=02H 表示传感器上无手指；
             确认码=03H 表示录入不成功 
  */
int16_t ZN632_GetImage( void ){
    uint16_t temp = 0;
    int16_t  ret = 0;
    uint8_t cmd[6] = {0};
 
    cmd[0] = 0x01;         //包标识
    cmd[1] = 0x00;         //包长度
    cmd[2] = 0x03;         //包长度
    cmd[3] = CMD_GetImage; //命令码
    temp = cmd[0] + cmd[1] + cmd[2] + cmd[3];
    cmd[4] = temp >> 8; 
    cmd[5] = temp;
    
    ZN632_SendHead();
    ZN632_SendData(cmd, sizeof(cmd));

    ZN632_DELAY_MS( 500 );//等待模块处理数据

    ret = ZN632_CheckAck();
    if(ret != 0){//显示错误信息
        ZN632_ShowError(ret);
    }
    
    ZN632_ClearRxBuf();
    return ret;
}

/**
  * @brief  将ImageBuffer中的原始图像生成指纹特征文件存于CHARBUFFER1或CHARBUFFER2
  * @param  BufferID(特征缓冲区号)
  * @retval 确认码=00H 表示生成特征成功；确认码=01H 表示收包有错；
            确认码=06H 表示指纹图像太乱而生不成特征；
            确认码=07H 表示指纹图像正常，但特征点太少而生不成特征；
            确认码=15H 表示图像缓冲区内没有有效原始图而生不成图像
  */
int16_t ZN632_GenChar(uint8_t BufferID ){
    uint16_t temp = 0;
    int16_t  ret = 0;
    uint8_t  cmd[7] = {0};

    cmd[0] = 0x01;         //包标识
    cmd[1] = 0x00;         //包长度
    cmd[2] = 0x04;         //包长度
    cmd[3] = CMD_GenChar;  //命令码
    cmd[4] = BufferID;
    temp = cmd[0] + cmd[1] + cmd[2] + cmd[3] + cmd[4]; //校验和
    cmd[5] = temp >> 8; 
    cmd[6] = temp ;
    
    ZN632_SendHead();
    ZN632_SendData(cmd, sizeof(cmd));
       
    ZN632_DELAY_MS( 500 );
    
    ret = ZN632_CheckAck();
    if(ret != 0){//显示错误信息
        ZN632_ShowError( ret );
    }
       
    ZN632_ClearRxBuf();
    return ret;
}

/**
  * @brief  将CHAR BUFFER1与CHARBUFFER2 中的特征文件合并生成模板，
            结果存于CHARBUFFER1与CHARBUFFER2。
  * @param  无
  * @retval 
确认码=00H 表示合并成功；
确认码=01H 表示收包有错；
确认码=0aH 表示合并失败（两枚指纹不属于同一手指）
  */
int16_t ZN632_RegModel( void ){
    uint16_t temp = 0;
    int16_t  ret = 0;
    uint8_t cmd[6] = {0};

    cmd[0] = 0x01;         //包标识
    cmd[1] = 0x00;         //包长度
    cmd[2] = 0x03;         //包长度
    cmd[3] = CMD_RegModel;  //命令码
    temp = cmd[0] + cmd[1] + cmd[2] + cmd[3]; //校验和
    cmd[4] = temp >> 8;
    cmd[5] = temp ;
      
    ZN632_SendHead();
    ZN632_SendData(cmd, 6);

    ZN632_DELAY_MS(500);
    
    ret = ZN632_CheckAck();
    if(ret != 0){//显示错误信息
        ZN632_ShowError(temp);
    }

    ZN632_ClearRxBuf();  
    return  ret;
}

/**
  * @brief  将 CHARBUFFER1 或 CHARBUFFER2 中的模板文件存到 PageID 号flash 数据库位置。
  * @param  BufferID:缓冲区号，
  * @param     PageID:指纹库位置号
  * @retval 确认码=00H 表示储存成功；
            确认码=01H 表示收包有错；
            确认码=0bH 表示 PageID 超出指纹库范围；
            确认码=18H 表示写 FLASH 出错
  */
int16_t ZN632_StoreChar(uint8_t BufferID, uint16_t PageID){
    uint16_t temp = 0;
    int16_t ret = 0;
    uint8_t cmd[9] = {0};

    cmd[0] = 0x01;  //包标识
    cmd[1] = 0x00;  //包长度
    cmd[2] = 0x06;  //包长度
    cmd[3] = CMD_StoreChar;//命令码
    cmd[4] = BufferID;  //BufferID
    cmd[5] = PageID>>8;  //PageID
    cmd[6] = PageID;
    
    temp = cmd[0] + cmd[1] + cmd[2] + cmd[3] + cmd[4] + cmd[5] + cmd[6]; //校验和
    cmd[7] = temp >> 8;
    cmd[8] = temp ;

    ZN632_SendHead();
    ZN632_SendData( cmd, sizeof(cmd));
    
    ZN632_DELAY_MS( 300 );
    
    ret = ZN632_CheckAck();
    if(ret != 0){//显示错误信息
        ZN632_ShowError(ret);
    }
    
    ZN632_ClearRxBuf();
    return  ret;
}

/**
  * @brief  以CHAR BUFFER1或CHAR BUFFER2中的特征文件高速搜索整个或部分指纹库
  * @param  BufferID:缓冲区号
  * @param  StartPage:起始页
  * @param  PageNum:页数
  * @retval 
    ensure:确认字,
    *p:页码（相配指纹模板）
  */
int16_t ZN632_HighSpeedSearch(uint8_t BufferID, uint16_t *pID, uint16_t *pScore)
{
    uint16_t temp = 0;
    int16_t ret = 0;
    uint8_t cmd[11] = {0};

    cmd[0] = 0x01;         //包标识    
    cmd[1] = 0x00;         //包长度
    cmd[2] = 0x08;         //包长度
    cmd[3] = CMD_HighSpeedSearch; //命令码
    cmd[4] = BufferID;  //BufferID
    cmd[5] = 0;  //StartPage
    cmd[6] = 0;
    cmd[7] = ZN632_INDEX_MAX>>8;
    cmd[8] = ZN632_INDEX_MAX;
    
    temp = cmd[0] + cmd[1] + cmd[2] + cmd[3] + cmd[4] + cmd[5] + cmd[6] + cmd[7] + cmd[8]; //校验和
    cmd[9]  = temp >> 8;
    cmd[10] = temp ;

    ZN632_SendHead();
    ZN632_SendData(cmd, sizeof(cmd));
    
     ZN632_DELAY_MS( 300 );
    
    ret = ZN632_CheckAck();
    if(ret != 0){
        ZN632_ShowError( ret );
        goto _END;
    }
 
    *pID    = ZN632_RxBuf[10]<<8 | ZN632_RxBuf[11];
    *pScore = ZN632_RxBuf[12]<<8 | ZN632_RxBuf[13];

_END:
    ZN632_ClearRxBuf();
    return  ret;
}

/*
阻塞获取手指图像，
可通过中断或其他任务退出阻塞，以便切换其他任务
*/
int8_t FPR_GetImage( void ){
    while(1){
        if(ZN632_GetImage() == 0){
            printf("Get One Finger\r\n");
            return 0;
        }
        
        if(g_uExitToChange == 1 ){//退出切换任务
            g_uExitToChange = 0;
            return -1;
        }
    } 
}

/**
  * @brief  录入指纹
  * @param  无
  * @retval 无
  */
void FPR_AddFinger( void ){
    int16_t ret = 0; 
    uint16_t uID    = 0;
    uint16_t uScore = 0;
    uint8_t uStep = 1;
 
    while(uStep <= 2){//采集两个指纹    
        printf("Put Finger On Senser:%d\r\n", uStep);
    
        ret = FPR_GetImage(); //录入图像  
        if(ret != 0){ return; }

        ret = ZN632_GenChar(uStep);  //生成特征1
        if(ret != 0){ continue; }

        ret = ZN632_HighSpeedSearch(uStep, &uID, &uScore);
        if(ret == 0) {
            printf("Finger is Already registed\r\n");
            continue;
        }
    
        uStep++;
    }

    ret = ZN632_RegModel();//合并特征，生成模板
    if(ret != 0){ return;  }
    
    uID = ZN632_GetIndexEmpty(); //获取空的编号位置
    ret = ZN632_StoreChar(2, uID); //储存模板
    if(ret != 0){ return; }
    ZN632_SetIndex(uID); //保存成功，设置标识位

    printf("Add Finger OK,ID:%d\r\n", uID); 
}

/**
  * @brief  比对指纹（识别指纹）
  * @param  无
  * @retval 无
  */
void FPR_MatchFinger( void ) {  
    uint16_t uID    = 0;                    //初始化ID值
    uint16_t uScore = 0; 
    int16_t  temp = 0;

    printf("Put Finger On Sensor:");
    temp = FPR_GetImage();
    if(temp != 0x00) { return; }
    
    temp = ZN632_GenChar(1);    //生成特征1
    if(temp != 0) {  return; }
   
    temp = ZN632_HighSpeedSearch(1, &uID, &uScore); //高速搜索指纹库
    if(temp != 0) { 
        printf("Finger NOT Match\r\n");
        return;
    }
    
    printf("Finger Match, ID:%d\r\n", uID);
}

/**
  * @brief  显示错误信息
  * @param  ErrCode:确认码
  * @retval 无
  */
void  ZN632_ShowError(uint16_t  code) {

#if 0
    switch( code ){

    case 0x00:
        printf("OK\r\n");
        break;        
    case 0x01:
        printf("数据包接收错误\r\n");
        break;
    case 0x02:
        printf("指纹模块没有检测到指纹！\r\n");
        break;
    case  0x03:
        printf("录入指纹图像失败\r\n");
        break;
    case  0x04:
        printf("指纹图像太干、太淡而生不成特征\r\n");
        break;      
    case  0x05:
        printf("指纹图像太湿、太糊而生不成特征\r\n");
        break;
    case  0x06:
        printf("指纹图像太乱而生不成特征\r\n");
        break;    
    case  0x07:
        printf("指纹图像正常，但特征点太少（或面积太小）而生不成特征\r\n");
        break;
    case  0x08:
        printf("指纹不匹配\r\n");
        break;        
    case  0x09:
        printf("对比指纹失败，指纹库不存在此指纹！\r\n");
        break;        
    case  0x0a:
        printf("特征合并失败\r\n");
        break;
    case  0x0b:
        printf("访问指纹库时地址序号超出指纹库范围\r\n");
        break;        
    case  0x10:
        printf("删除模板失败\r\n");
        break;
    case  0x11:
        printf("清空指纹库失败\r\n");
        break;            
    case  0x15:
        printf("缓冲区内没有有效原始图而生不成图像\r\n");
        break;        
    case  0x18:
        printf("读写 FLASH 出错\r\n");
        break;        
    case  0x19:
        printf("未定义错误\r\n");
        break;        
    case  0x1a:
        printf("无效寄存器号\r\n");
        break;
    case  0x1b:
        printf("寄存器设定内容错误\r\n");
        break;
    case  0x1c:
        printf("记事本页码指定错误\r\n");
        break;
    case  0x1f:
        printf("指纹库满\r\n");
        break;
    case  0x20:
        printf("地址错误\r\n");
        break;        
    default :
        printf("模块返回确认码有误\r\n");
        break;    
    }
#endif
}
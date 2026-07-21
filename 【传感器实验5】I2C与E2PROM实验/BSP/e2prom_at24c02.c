#include <stdio.h>
#include "e2prom_at24c02.h"
#include "usart.h"
#include "delay.h"


#define AT24C02_I2C I2C1 //I2C接口

#define PAGE_SIZE 8 //每页字节数
#define PAGE_COUNT 32 //总页数
#define TOTAL_SIZE 256 //总字节数

#define I2C_Speed 100000 //STM32 I2C 快速模式

uint16_t AT24C02_ADDRESS = 0xA0;

int8_t AT24C02_WaitI2cEvent( uint32_t I2C_EVENT );
int8_t AT24C02_WaitI2cFlag(uint32_t I2C_FLAG, FlagStatus Stauts);
void AT24C02_WaitE2promOK( void );

/**
  * @brief  I2C1 I/O配置
  * @param  无
  * @retval 无
  */
static void I2C_GPIO_Config( void ){
    GPIO_InitTypeDef  GPIO_InitStructure;    

    //GPIO时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    //SCL:PB6，SDA:PB7
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6|GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    //GPIO复用设置
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_I2C1);//SCL   
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_I2C1);//SDA
}

/**
  * @brief  I2C工作模式配置
  * @param  无
  * @retval 无
  */
static void I2C_Mode_Config(void){
    I2C_InitTypeDef  I2C_Struct; 

    I2C_Struct.I2C_Mode        = I2C_Mode_I2C;    
    I2C_Struct.I2C_DutyCycle   = I2C_DutyCycle_2; //SCL时钟线的占空比
    //I2C_Struct.I2C_OwnAddress1 = 0x0A; //作为从设备时的地址，作主设备可用任意值 
    I2C_Struct.I2C_Ack = I2C_Ack_Enable;
    I2C_Struct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;//I2C的寻址模式
    I2C_Struct.I2C_ClockSpeed = I2C_Speed;//通信速率
    I2C_Init(AT24C02_I2C, &I2C_Struct);//I2C1初始化

    I2C_Cmd(AT24C02_I2C, ENABLE); //使能 I2C1
    I2C_AcknowledgeConfig(AT24C02_I2C, ENABLE);  
}

/**
  * @brief  I2C 外设(EEPROM)初始化
  * @param  无
  * @retval 无
  */
void AT24C02_Init(void){
  I2C_GPIO_Config(); 
  I2C_Mode_Config();
}

/*
等待指定I2C事件，超时未等到事件，则反回-1退出
*/
int8_t AT24C02_WaitI2cEvent( uint32_t I2C_EVENT ) {
    __IO uint32_t I2CTimeout = ((uint32_t)0x10000);
    
    while(!I2C_CheckEvent(AT24C02_I2C, I2C_EVENT)){ 
        if((I2CTimeout--) == 0) { return -1; } 
    }
    
    return 0;
}

/*
等待指定I2C标志位，超时未等到，则反回-1退出
*/
int8_t AT24C02_WaitI2cFlag( uint32_t I2C_FLAG, FlagStatus Status ) {
    __IO uint32_t I2CTimeout = ((uint32_t)0x100000);
    
    while(I2C_GetFlagStatus(AT24C02_I2C, I2C_FLAG) != Status){ 
        if((I2CTimeout--) == 0) {return -1;} 
    }
    
    return 0;
}

/**
  * @brief  等待E2PROM进入可操作状态
      E2PROM在执行写入擦除等命令后，需一定时间才可执行完，在这过程中不可接收新操作
      此函数用于等待E2PROM内部执行完毕，可继续操作
  * @param  无
  * @retval 无
  */
void AT24C02_WaitE2promOK( void ){
    __IO uint32_t uTimeout = 100;  //最多100ms
    
    do{
        if((uTimeout--) == 0) {return;} 
        delay_ms(1);

        I2C_GenerateSTART(AT24C02_I2C, ENABLE);//起始位
        if(AT24C02_WaitI2cEvent(I2C_EVENT_MASTER_MODE_SELECT) != 0){
            continue;
        }
        
        I2C_Send7bitAddress(AT24C02_I2C, AT24C02_ADDRESS, I2C_Direction_Transmitter);//发送地址
        if(AT24C02_WaitI2cEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) == 0){//状态OK
            break;
        }
    } while(1);
    
    I2C_ClearFlag(AT24C02_I2C, I2C_FLAG_AF);  //清除AF标识位
    I2C_GenerateSTOP(AT24C02_I2C, ENABLE);//停止位
}

/**
  * @brief   在EEPROM的一个写循环中可以写多个字节，但一次写入的字节数
  *          不能超过EEPROM页的大小，AT24C02每页有8个字节
  * @param   
  *        @arg pBuffer:缓冲区指针
  *        @arg WriteAddr:写地址
  *     @arg NumByteToWrite:写的字节数
  * @retval  无
  */
int8_t AT24C02_PageWrite(uint8_t* pBuffer, uint8_t WriteAddr, uint8_t Number){  
    if(AT24C02_WaitI2cFlag(I2C_FLAG_BUSY, RESET) != 0){return -1;} //等待I2C空闲
        
    I2C_GenerateSTART(AT24C02_I2C, ENABLE);      //发送起始位
    if(AT24C02_WaitI2cEvent(I2C_EVENT_MASTER_MODE_SELECT) != 0){return -1;}    //监测EV5事件

  
    //发送设备地址，请求写入数据
    I2C_Send7bitAddress(AT24C02_I2C, AT24C02_ADDRESS, I2C_Direction_Transmitter);
    if(AT24C02_WaitI2cEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) != 0){return -1;}//监测EV6事件

    I2C_SendData(AT24C02_I2C, WriteAddr);    //发送要写入数据的地址值
    if(AT24C02_WaitI2cEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != 0){return -1;} //监测EV8事件
    
    while( Number-- ){
        I2C_SendData(AT24C02_I2C, *pBuffer); //发送要写入的数据
        pBuffer++;
        if(AT24C02_WaitI2cEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != 0){return -1;} //监测EV8事件
    }

    //发送停止信号
    I2C_GenerateSTOP(AT24C02_I2C, ENABLE);

    AT24C02_WaitE2promOK();//等待写入执行完毕
    return 0;
}

/**
  * @brief   将缓冲区中的数据写到I2C EEPROM中
  * @param   
  *        @arg pBuffer:缓冲区指针
  *        @arg WriteAddr:写地址
  *     @arg NumByteToWrite:写的字节数
  * @retval  无
  */
void AT24C02_BufferWrite(uint8_t* pBuffer, uint8_t Addr, uint16_t Number){
    uint8_t PageIndex  = 0;
    uint8_t FirstCount = 0;
    uint8_t PageNum  = 0;
    uint8_t ByteNum  = 0;

    //写入字数不可大于剩余字数
    if(Number > (TOTAL_SIZE-Addr)) { Number = TOTAL_SIZE - Addr; }

    PageIndex  = Addr % PAGE_SIZE; //当前地址的页内位置
    FirstCount = PAGE_SIZE - PageIndex;
    if(FirstCount > Number){ FirstCount = Number; }

    // 若首页位置不是整页对齐，则先写首页
    if(PageIndex != 0){
        AT24C02_PageWrite(pBuffer, Addr, FirstCount);
        Addr     += FirstCount;
        pBuffer  += FirstCount;
        Number   -= FirstCount;
    }

    //剩余页数与最后一页字节数
    PageNum  = Number / PAGE_SIZE;
    ByteNum  = Number % PAGE_SIZE;

    while(PageNum > 0){
        AT24C02_PageWrite(pBuffer, Addr, PAGE_SIZE);
        Addr  +=  PAGE_SIZE;
        pBuffer +=  PAGE_SIZE;

        PageNum--;
    }
   
    //最后一页
    if(ByteNum > 0){
        AT24C02_PageWrite(pBuffer, Addr,ByteNum);
    }
}

/**
  * @brief   从EEPROM里面读取一块数据 
  * @param   
  *        @arg pBuffer:存放从EEPROM读取的数据的缓冲区指针
  *        @arg WriteAddr:接收数据的EEPROM的地址
  *     @arg NumByteToWrite:要从EEPROM读取的字节数
  * @retval  无
  */
int8_t AT24C02_BufferRead(uint8_t* pBuffer, uint8_t ReadAddr, uint16_t Number){
    
    if(AT24C02_WaitI2cFlag(I2C_FLAG_BUSY, RESET) != 0){return -1;} //等待I2C空闲

    I2C_GenerateSTART(AT24C02_I2C, ENABLE);//发送起始位
    if(AT24C02_WaitI2cEvent(I2C_EVENT_MASTER_MODE_SELECT) != 0){ return -1;} //监测EV5

    I2C_Send7bitAddress(AT24C02_I2C, AT24C02_ADDRESS, I2C_Direction_Transmitter);     //发送地址,请求写入
    if(AT24C02_WaitI2cEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) != 0){ return -1;} //监测EV6

    I2C_SendData(AT24C02_I2C, ReadAddr);    //发送要写入数据的地址值
    if(AT24C02_WaitI2cEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != 0){ return -1;} //监测EV8

    I2C_GenerateSTART(AT24C02_I2C, ENABLE);    //重新发送起始信号，开新读阶段
    if(AT24C02_WaitI2cEvent(I2C_EVENT_MASTER_MODE_SELECT) != 0){ return -1;} //监测EV5
  
    I2C_Send7bitAddress(AT24C02_I2C, AT24C02_ADDRESS, I2C_Direction_Receiver);    //发送设备地址，请求读数据
    if(AT24C02_WaitI2cEvent(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED) != 0){ return -1;} //监测EV6
    
    //开始读取数据
    while(Number){
        if(Number == 1){//最后一个字节
            I2C_AcknowledgeConfig(AT24C02_I2C, DISABLE);//不回复ACK
            I2C_GenerateSTOP(AT24C02_I2C, ENABLE); //发送停止位
        }

        if(AT24C02_WaitI2cEvent(I2C_EVENT_MASTER_BYTE_RECEIVED) != 0){return -1;}//监测EV7，RXNE为可接收状态
        *pBuffer = I2C_ReceiveData(AT24C02_I2C);//读取一字节数据
        
        pBuffer++; 
        Number--;
    }

    //重新使能ACK，以便其他后续操作
    I2C_AcknowledgeConfig(AT24C02_I2C, ENABLE);  
    return 0;
}
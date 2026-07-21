#include "flash_w25qxx.h"

#define W25QXX_FLASH_ID 0XEF4017 //W25Q64

#define W25QXX_PAGE_SIZE 256 //页大小
#define W25QXX_PER_WRITE_PAGE_SIZE 256 //页内每次可写入的最大值

#define CMD_WRITE_ENABLE 0x06
#define CMD_WRITE_DISABLE 0x04
#define CMD_READ_STATUS_REG 0x05
#define CMD_WRITE_STATUS_REG 0x01
#define CMD_READ_DATA 0x03
#define CMD_FAST_READ_DATA 0x0B
#define CMD_FAST_READ_DUAL 0x3B
#define CMD_PAGE_PROGRAM 0x02
#define CMD_BLOCK_ERASE 0xD8
#define CMD_SECTOR_ERASE 0x20
#define CMD_CHIP_ERASE 0xC7
#define CMD_POWER_DOWN 0xB9
#define CMD_RELEASE_POWER_DOWN 0xAB
#define CMD_DEVICE_ID 0xAB
#define CMD_MANU_DEVICE_ID 0x90
#define CMD_JEDEC_DEVICE_ID 0x9F

#define WIP_FLAG 0x01 //W25QXX的正在写入标示位
#define DUMMY_BYTE 0xFF


/* SPI接口定义-开头****************************/
#define W25QXX_SPI SPI1

#define W25QXX_CS_LOW() GPIO_ResetBits(GPIOC, GPIO_Pin_0); //CS拉低
#define W25QXX_CS_HIGH() GPIO_SetBits(GPIOC, GPIO_Pin_0); //CS拉高

int8_t W25QXX_WaitSpiFlag(uint32_t SPI_FLAG, FlagStatus Status);

/**
  * @brief  W25QXX初始化
  * @param  无
  * @retval 无
  */
void W25QXX_Init(void){
    SPI_InitTypeDef  SPI_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    //GPIO时钟使能，CS:PC0
    RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOA |RCC_AHB1Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);  //SPI时钟使能

    //SCK:PA5，MISO:PA6，MOSI:PA7
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    //CS:PC0
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    //设置引脚复用
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_SPI1); 
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_SPI1); 
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_SPI1); 

    W25QXX_CS_HIGH();  //停止信号 FLASH: CS引脚高电平

    //SPI接口设置
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_High; //W25QXX支持SPI模式0及模式3
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(W25QXX_SPI, &SPI_InitStructure);

    //使能SPI
    SPI_Cmd(W25QXX_SPI, ENABLE);
}

 /**
  * @brief  使用SPI读取一个字节的数据
  * @param  无
  * @retval 返回接收到的数据
  */
uint8_t W25QXX_ReadByte(void){
  return (W25QXX_SendByte( DUMMY_BYTE ));
}


/**
* @brief  使用SPI发送一个字节的数据
* @param  byte：要发送的数据
* @retval 返回接收到的数据
*/
uint8_t W25QXX_SendByte(uint8_t byte){
    if(W25QXX_WaitSpiFlag(SPI_I2S_FLAG_TXE, SET) != 0){return 0;} //等待可发送标识
    SPI_I2S_SendData(W25QXX_SPI, byte); //写入数据寄存器

    if(W25QXX_WaitSpiFlag(SPI_I2S_FLAG_RXNE, SET) != 0){return 0;} //等待可接收标识
    return SPI_I2S_ReceiveData( W25QXX_SPI );//读取数据
}

/*
等待指定SPI标志位，超时未等到，则反回-1退出
*/
int8_t W25QXX_WaitSpiFlag( uint32_t SPI_FLAG, FlagStatus Status ) {
    __IO uint32_t uTimeout = ((uint32_t)0x100000);
    
    while(SPI_I2S_GetFlagStatus(W25QXX_SPI, SPI_FLAG) != Status){ 
        if((uTimeout--) == 0) {return -1;} 
    }
    
    return 0;
}

/**
  * @brief  读取FLASH ID
  * @param     无
  * @retval FLASH ID
  */
uint32_t W25QXX_ReadID(void){
    uint32_t Temp,Temp0,Temp1,Temp2 = 0;

    W25QXX_CS_LOW();  //使能开始通讯

    W25QXX_SendByte( CMD_JEDEC_DEVICE_ID );  //发送JEDEC指令，读取ID

    Temp0 = W25QXX_SendByte( DUMMY_BYTE ); 
    Temp1 = W25QXX_SendByte( DUMMY_BYTE ); 
    Temp2 = W25QXX_SendByte( DUMMY_BYTE );

    W25QXX_CS_HIGH();  //停止通讯

    Temp = (Temp0 << 16) | (Temp1 << 8) | Temp2;     //接拼数据
    return Temp;
}

/**
  * @brief  向FLASH发送 写使能 命令
  * @param  none
  * @retval none
  */
void W25QXX_WriteEnable(void){
    W25QXX_CS_LOW();  //通讯开始
    W25QXX_SendByte( CMD_WRITE_ENABLE );  //发送写使能命令
    W25QXX_CS_HIGH();  //通讯结束
}

/**
  * @brief  等待WIP(BUSY)标志被置0，即等待到FLASH内部数据写入完毕
  * @param  none
  * @retval none
  */
void W25QXX_WaitForWriteEnd( void ){
    u8 FLASH_Status = 0;
    __IO uint32_t uTimeout = ((uint32_t)0x100000);

    W25QXX_CS_LOW(); //选择FLASH
    W25QXX_SendByte( CMD_READ_STATUS_REG );  //读状态寄存器 命令

    do {
        FLASH_Status = W25QXX_SendByte( DUMMY_BYTE );          //读取FLASH芯片的状态寄存器
        if((uTimeout--) == 0) {break;}
    }
    while((FLASH_Status & WIP_FLAG ) == SET); //正在写入表示未写完

    W25QXX_CS_HIGH();  //停止FLASH
}

/**
  * @brief  擦除FLASH扇区
  * @param  SectorAddr：要擦除的扇区地址
  * @retval 无
  */
void W25QXX_SectorErase(uint32_t SectorAddr)
{
    W25QXX_WriteEnable();      //发送FLASH写使能命令
    W25QXX_CS_LOW();  //使能FLASH:

    W25QXX_SendByte( CMD_SECTOR_ERASE );//发送扇区擦除指令
    W25QXX_SendByte((SectorAddr & 0xFF0000) >> 16);//发送擦除扇区地址的高位
    W25QXX_SendByte((SectorAddr & 0xFF00) >> 8);    //发送擦除扇区地址的中位
    W25QXX_SendByte(SectorAddr & 0xFF);     //发送擦除扇区地址的低位

    W25QXX_CS_HIGH();    //停止信号 FLASH

    W25QXX_WaitForWriteEnd();  /* 等待擦除完毕*/
}

/**
  * @brief  对FLASH按页写入数据，调用本函数写入数据前需要先擦除扇区
  * @param  pBuffer 要写入数据的指针
  * @param  WriteAddr 写入地址
  * @param  NumByteToWrite 写入数据长度，必须小于等于W25QXX_PerWritePageSize
  * @retval 无
  */
void W25QXX_PageWrite(u8* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite)
{
    W25QXX_WriteEnable();  //使能FLASH
 
    W25QXX_CS_LOW();  //选择FLASH

    W25QXX_SendByte( CMD_PAGE_PROGRAM );   //写页写指令
    W25QXX_SendByte((WriteAddr & 0xFF0000) >> 16);  //发送写地址的高位
    W25QXX_SendByte((WriteAddr & 0xFF00) >> 8);   //发送写地址的中位
    W25QXX_SendByte(WriteAddr & 0xFF);    //发送写地址的低位

    if(NumByteToWrite > W25QXX_PER_WRITE_PAGE_SIZE){//大于可写入的最大值，则只写入部分
        NumByteToWrite = W25QXX_PER_WRITE_PAGE_SIZE;
        printf( "W25QXX_PageWrite too large!" );
    }

    while(NumByteToWrite--){
        W25QXX_SendByte(*pBuffer); //发送当前写入数据
        pBuffer++;
    }

  W25QXX_CS_HIGH();  //停止信号FLASH
    
  W25QXX_WaitForWriteEnd();   /* 等待写入完毕*/
}

void W25QXX_BufferWrite(uint8_t*pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite) {
    uint8_t NumOfPage = 0, NumOfSingle = 0, PageIndex = 0, FirstCount = 0 ;

    PageIndex  = WriteAddr % W25QXX_PAGE_SIZE;
    FirstCount = W25QXX_PAGE_SIZE - PageIndex;
    if(FirstCount > NumByteToWrite){ FirstCount = NumByteToWrite;}

    // 若首页位置不是整页，则先写一页去除不对齐
    if(PageIndex != 0) {
        W25QXX_PageWrite(pBuffer, WriteAddr, FirstCount);
        WriteAddr +=  FirstCount;
        pBuffer += FirstCount;
        NumByteToWrite -= FirstCount;
    }

    NumOfPage   = NumByteToWrite / W25QXX_PAGE_SIZE;
    NumOfSingle = NumByteToWrite % W25QXX_PAGE_SIZE;

    //　写中间整页
    while(NumOfPage > 0) {
        W25QXX_PageWrite(pBuffer, WriteAddr, W25QXX_PAGE_SIZE);
        WriteAddr += W25QXX_PAGE_SIZE;
        pBuffer   += W25QXX_PAGE_SIZE;

        NumOfPage--;
    }

    // 写最后一页
    if (NumOfSingle != 0) {
        W25QXX_PageWrite(pBuffer, WriteAddr, NumOfSingle);
    }
}

/**
  * @brief  读取FLASH数据
  * @param     pBuffer，存储读出数据的指针
  * @param   ReadAddr，读取地址
  * @param   NumByteToRead，读取数据长度
  * @retval 无
  */
void W25QXX_BufferRead(u8* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead)
{
  W25QXX_CS_LOW();  //选择FLASH

  W25QXX_SendByte( CMD_READ_DATA );  //发送 读 指令
  W25QXX_SendByte((ReadAddr & 0xFF0000) >> 16);  //发送 读地址高位
  W25QXX_SendByte((ReadAddr& 0xFF00) >> 8);  //发送读地址中位
  W25QXX_SendByte(ReadAddr & 0xFF);

  while(NumByteToRead--) {    //循环读取数据
    *pBuffer = W25QXX_SendByte( DUMMY_BYTE );     //读取一个字节
    pBuffer++; 
  }

  W25QXX_CS_HIGH();  //停止信号FLASH
}
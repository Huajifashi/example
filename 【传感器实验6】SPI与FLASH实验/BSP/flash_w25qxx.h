#ifndef __FLASH_W25QXX_H__
#define __FLASH_W25QXX_H__

#include <stdio.h>
#include "stm32f4xx.h"


void W25QXX_Init( void );
uint32_t  W25QXX_ReadID( void );
uint32_t  W25QXX_ReadDeviceID( void );
void W25QXX_SectorErase( u32 SectorAddr );
void W25QXX_BulkErase( void );
void W25QXX_PageWrite( u8* pBuffer, u32 WriteAddr, u16 NumByteToWrite );
void W25QXX_BufferWrite( u8* pBuffer, u32 WriteAddr, u16 NumByteToWrite );
void W25QXX_BufferRead( u8* pBuffer, u32 ReadAddr, u16 NumByteToRead );
void W25QXX_StartReadSequence( u32 ReadAddr );
void W25QXX_PowerDown( void );
void W25QXX_WakeUp( void );

uint8_t   W25QXX_ReadByte( void );
uint8_t   W25QXX_SendByte( u8 byte );
void W25QXX_WriteEnable( void );
void W25QXX_WaitForWriteEnd( void );

#endif /* __FLASH_W25QXX_H__ */
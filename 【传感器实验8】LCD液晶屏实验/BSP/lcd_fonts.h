#ifndef __LCD_FONT_H__
#define __LCD_FONT_H__

#include "stm32f4xx.h"

typedef struct _tFont{
  const uint8_t *table;
  uint16_t Width;
  uint16_t Height;
}FONT;

extern FONT ASCII_8x16;   //英文ASCII字体
#define LINE_EN(x) ((x)*(((FONT *)LCD_GetFontEN())->Height))

FONT *LCD_GetFontEN(void);
void  LCD_SetFontEN(FONT *fonts);

uint8_t *LCD_GetMaskEN(char cChar); //获取英文字模

#endif 
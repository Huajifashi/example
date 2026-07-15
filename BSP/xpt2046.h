#ifndef __XPT2046_H__
#define __XPT2046_H__
#include "stm32f4xx.h"

typedef struct{          //液晶坐标结构体
   // 负数值表示无新数据
   int16_t x;                   //记录最新的触摸参数值
   int16_t y;

   // 用于记录连续触摸时(长按)的上一次触摸位置
   int16_t pre_x;
   int16_t pre_y;
} XPT2046_Coord;

typedef struct{ //校准因子结构体
  long double A;
  long double B;
  long double C;
  long double D;
  long double E;
  long double F;
} XPT2046_Factor;

typedef enum {
   XPT2046_STATE_RELEASE  = 0,
   XPT2046_STATE_WAITING,
   XPT2046_STATE_PRESSED,
}enumTouchState;


void    XPT2046_Init( void );
uint8_t XPT2046_DetectTouch( void );
int8_t XPT2046_GetTouchPoint(XPT2046_Coord *displayPtr);
int8_t XPT2046_CalibrationFactor(XPT2046_Coord *pLCD, XPT2046_Coord *pTouch, XPT2046_Factor *pFactor);

#endif /* __XPT2046_H__ */
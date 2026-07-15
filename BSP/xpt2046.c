#include <stdio.h>
#include <string.h>
#include "xpt2046.h"
#include "delay.h"

#define XPT2046_CHANNEL_X 0x90 //通道Y+的选择控制字
#define XPT2046_CHANNEL_Y 0xd0 //通道X+的选择控制字

#define TOUCH_PRESSED 1
#define TOUCH_NOT_PRESSED 0

#define XPT2046_PENIRQ_Active 0
#define XPT2046_THRESHOLD_CalDiff 2 //校准触摸屏时触摸坐标的AD值相差门限
#define DURIATION_TIME 2


#define XPT2046_CS_ENABLE() GPIO_ResetBits(GPIOD, GPIO_Pin_13)
#define XPT2046_CS_DISABLE() GPIO_SetBits(GPIOD, GPIO_Pin_13 )

#define XPT2046_CLK_HIGH() GPIO_SetBits(GPIOE, GPIO_Pin_0 )
#define XPT2046_CLK_LOW() GPIO_ResetBits(GPIOE, GPIO_Pin_0 )

#define XPT2046_MOSI_1() GPIO_SetBits(GPIOE, GPIO_Pin_2 )
#define XPT2046_MOSI_0() GPIO_ResetBits( GPIOE, GPIO_Pin_2 )

#define XPT2046_MISO() GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_3 )
#define XPT2046_ReadIRQ() GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_4 )

//微秒延时函数
#define XPT2046_DelayUS(x) delay_us(x)



//当前使用的LCD扫描方式，需与LCD驱动的值一致
int LCD_SCAN_MODE = 0;
/******************************* 定义 XPT2046 全局变量 ***************************/
//默认触摸参数，不同的屏幕稍有差异，可重新调用触摸校准函数获取
XPT2046_Factor TouchFactor[] = {
-0.006464, -0.073259, 280.358032, 0.074878, 0.002052, -6.545977,//扫描方式0
0.086314, 0.001891, -12.836658, -0.003722, -0.065799, 254.715714,//扫描方式1
0.002782, 0.061522, -11.595689, 0.083393, 0.005159, -15.650089,//扫描方式2
0.089743, -0.000289, -20.612209, -0.001374, 0.064451, -16.054003,//扫描方式3
0.000767, -0.068258, 250.891769, -0.085559, -0.000195, 334.747650,//扫描方式4
-0.084744, 0.000047, 323.163147, -0.002109, -0.066371, 260.985809,//扫描方式5
-0.001848, 0.066984, -12.807136, -0.084858, -0.000805, 333.395386,//扫描方式6
-0.085470, -0.000876, 334.023163, -0.003390, 0.064725, -6.211169,//扫描方式7
};

/**
  * @brief  XPT2046 初始化函数
  * @param  无
  * @retval 无
  */    
void XPT2046_Init( void ){
  GPIO_InitTypeDef  GPIO_InitStructure;

  //开启GPIO时钟
  RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOD | RCC_AHB1Periph_GPIOE, ENABLE );

  //SPI CLK          
  GPIO_InitStructure.GPIO_Pin    = GPIO_Pin_0;
  GPIO_InitStructure.GPIO_Speed  = GPIO_Speed_25MHz ;      
  GPIO_InitStructure.GPIO_Mode   = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType  = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd   = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOE, &GPIO_InitStructure);

  //SPI MOSI
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
  GPIO_Init(GPIOE, &GPIO_InitStructure);

  //SPI CS
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_13; 
  GPIO_Init(GPIOD, &GPIO_InitStructure); 

  //MISO
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_3; 
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz ;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;  
  //GPIO_InitStructure.GPIO_OType  = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd   = GPIO_PuPd_UP;
  GPIO_Init(GPIOE, &GPIO_InitStructure);

  //IRQ
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_4;       
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;     // 上拉输入
  //GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;  
  GPIO_Init(GPIOE, &GPIO_InitStructure);    

  //拉低片选，选择XPT2046
  XPT2046_CS_ENABLE();
}

/**
  * @brief  XPT2046 的写入命令
  * @param  ucCmd ：命令
  *   该参数为以下值之一：
  *     @arg 0x90 :通道Y+的选择控制字
  *     @arg 0xd0 :通道X+的选择控制字
  * @retval 无
  */
static void XPT2046_WriteCMD( uint8_t ucCmd ) {
    uint8_t i = 0;

    XPT2046_MOSI_0();
    XPT2046_CLK_LOW();

    for(i = 0; i < 8; i ++){
     ((ucCmd >> ( 7 - i)) & 0x01)?XPT2046_MOSI_1() : XPT2046_MOSI_0();    
      XPT2046_DelayUS(5);
      XPT2046_CLK_HIGH();
      XPT2046_DelayUS ( 5 );
      XPT2046_CLK_LOW();
    }
}


/**
  * @brief  XPT2046 的读取命令
  * @param  无
  * @retval 读取到的数据
  */
static uint16_t XPT2046_ReadCMD ( void ) {
    uint8_t i;
    uint16_t usBuf=0, usTemp;
    
    XPT2046_MOSI_0();
    XPT2046_CLK_HIGH();

    for ( i=0;i<12;i++ ) {
        XPT2046_CLK_LOW();    
        usTemp = XPT2046_MISO();
        usBuf |= usTemp << ( 11 - i );
        XPT2046_CLK_HIGH();
    }
    return usBuf;
}

/**
  * @brief  对 XPT2046 选择一个模拟通道后，启动ADC，并返回ADC采样结果
  * @param  ucChannel
  *   该参数为以下值之一：
  *     @arg 0x90 :通道Y+的选择控制字
  *     @arg 0xd0 :通道X+的选择控制字
  * @retval 该通道的ADC采样结果
  */
static uint16_t XPT2046_ReadAdc( uint8_t ucChannel ){
  XPT2046_WriteCMD( ucChannel );
  return  XPT2046_ReadCMD();
}

/**
  * @brief  读取 XPT2046 的X通道和Y通道的AD值（12 bit，最大是4096）
  * @param  sX_Ad ：存放X通道AD值的地址
  * @param  sY_Ad ：存放Y通道AD值的地址
  * @retval 无
  */
static void XPT2046_ReadAdcXY(uint16_t *sX_Ad, uint16_t *sY_Ad)  {
    *sX_Ad = XPT2046_ReadAdc( XPT2046_CHANNEL_X );
    XPT2046_DelayUS( 1 );
    *sY_Ad = XPT2046_ReadAdc( XPT2046_CHANNEL_Y );
}

static int8_t XPT2046_FilterXY( XPT2046_Coord *pCoord ){
    uint8_t ucCount = 0, i = 0;
    uint16_t value = 0;
    uint16_t tempAD_X, tempAD_Y = 0;  //X，Y的临时值
    uint16_t sBufferArray[2][10] = { { 0 },{ 0 } };  //坐标X和Y进行多次采样
    uint32_t lX_Min, lX_Max, lY_Min, lY_Max = 0;    //存储采样中的最小值、最大值

    do{// 循环采样10次
        XPT2046_ReadAdcXY( &tempAD_X, &tempAD_Y );
        sBufferArray[0][ucCount] = tempAD_X;
        sBufferArray[1][ucCount] = tempAD_Y;

        ucCount ++;
        value = XPT2046_ReadIRQ();
    }while( (value == XPT2046_PENIRQ_Active ) && ( ucCount < 10 ) );


    if( ucCount < 10 )    {    // 未采样10个样本，无效不做处理
        return -1;
    }

    lX_Max = lX_Min = sBufferArray[0][0];
    lY_Max = lY_Min = sBufferArray[1][0];
    tempAD_X = sBufferArray[0][0]; //用于求和，XPT2046为ADC为12位,tempAD_X/Y为16位，保存10次和不会溢出
    tempAD_Y = sBufferArray[1][0];
    //找出X轴最小最大值
    for ( i = 1; i < 10; i ++ ){
        if ( sBufferArray[0][i] < lX_Min ){
            lX_Min = sBufferArray[0][i];
        }
        else if (sBufferArray[0][i] > lX_Max ){
            lX_Max = sBufferArray[0][i];
        }

        tempAD_X += sBufferArray[0][i];
    }

    //找出Y轴最小最大值
    for ( i = 1; i < 10; i ++ ){
        if ( sBufferArray[1][i] < lY_Min ){
            lY_Min = sBufferArray[1][i];
        }
        else if ( sBufferArray[1][i] > lY_Max ){
            lY_Max = sBufferArray[1][i];
        }

        tempAD_Y += sBufferArray[1][i];
    }

    // 去除最小值和最大值之后求平均值
    pCoord->x = (tempAD_X - lX_Min - lX_Max) >> 3;
    pCoord->y = (tempAD_Y - lY_Min - lY_Max) >> 3;

    return 0;
}

/**
  * @brief  获取 XPT2046 触摸点（校准后）的坐标
  * @param  pLCD ：该指针存放获取到的触摸点坐标
  */
int8_t XPT2046_GetTouchPoint(XPT2046_Coord *pLCD ){
    XPT2046_Coord adcCoord;

    if( XPT2046_FilterXY( &adcCoord ) < 0 ){ return -1; }

    // XL = AX + BY + C
    pLCD->x = TouchFactor[LCD_SCAN_MODE].A * adcCoord.x +
              TouchFactor[LCD_SCAN_MODE].B * adcCoord.y +
              TouchFactor[LCD_SCAN_MODE].C ;

    // YL = DX + EY + F;
    pLCD->y = TouchFactor[LCD_SCAN_MODE].D * adcCoord.x +
              TouchFactor[LCD_SCAN_MODE].E * adcCoord.y +
              TouchFactor[LCD_SCAN_MODE].F ;
    return 0;
}

//触摸屏按下时的调用函数
/**
  * @brief   触摸屏被按下的时候会调用本函数
  * @param  touch 包含触摸坐标的结构体
  * @note  请在本函数中编写自己的触摸按下处理应用
  * @retval 无
  */
__weak void XPT2046_TouchDown(XPT2046_Coord *touch){
    printf("TouchDown,x:%d,y:%d\r\n", touch->x, touch->y);
}

/**
  * @brief   触摸屏释放的时候会调用本函数
  * @param  touch 包含触摸坐标的结构体
  * @note  请在本函数中编写自己的触摸释放处理应用
  * @retval 无
  */
__weak void XPT2046_TouchUp(XPT2046_Coord *touch) {
    printf("TouchUp,x:%d,y:%d\r\n", touch->x, touch->y);
}

/**
  * @brief  触摸屏检测状态机
  * @retval 触摸状态
  * 该返回值为以下值之一：
  * @arg TOUCH_PRESSED :触摸按下
  * @arg TOUCH_NOT_PRESSED :无触摸
  */
uint8_t XPT2046_DetectTouch(void){
    uint8_t detectResult = TOUCH_NOT_PRESSED;
    uint16_t value = 0;
    static uint32_t i = 0;
    static  enumTouchState touch_state = XPT2046_STATE_RELEASE;
    static XPT2046_Coord cinfo = {-1,-1,-1,-1};

    value = XPT2046_ReadIRQ();
    switch( touch_state ){
        case XPT2046_STATE_RELEASE:
            if(value == XPT2046_PENIRQ_Active){ //第一次出现触摸信号
                touch_state  = XPT2046_STATE_WAITING;
                detectResult = TOUCH_NOT_PRESSED;
            }
            else {    //无触摸
                touch_state  = XPT2046_STATE_RELEASE;
                detectResult = TOUCH_NOT_PRESSED;
            }
            break;
        case XPT2046_STATE_WAITING:
            if(value == XPT2046_PENIRQ_Active){
                i++;
                //等待时间大于阈值则认为触摸被按下
                //消抖时间 = DURIATION_TIME * 本函数被调用的时间间隔
                //如在定时器中调用，每10ms调用一次，则消抖时间为：DURIATION_TIME*10ms
                if(i > DURIATION_TIME){
                    i = 0;
                    touch_state  = XPT2046_STATE_PRESSED;
                    detectResult = TOUCH_PRESSED;
                }
                else {//等待时间累加
                    touch_state  = XPT2046_STATE_WAITING;
                    detectResult = TOUCH_NOT_PRESSED;
                }
            }
            else {    //等待时间值未达到阈值就为无效电平，当成抖动处理
                i = 0;
                touch_state  = XPT2046_STATE_RELEASE;
                detectResult = TOUCH_NOT_PRESSED;
            }
            break;

        case XPT2046_STATE_PRESSED:
            if(value == XPT2046_PENIRQ_Active){        //触摸持续按下
                touch_state  = XPT2046_STATE_PRESSED;
                detectResult = TOUCH_PRESSED;
            }
            else {//触摸释放
                touch_state  = XPT2046_STATE_RELEASE;
                detectResult = TOUCH_NOT_PRESSED;
            }
            break;
        default:
            touch_state  = XPT2046_STATE_RELEASE;
            detectResult = TOUCH_NOT_PRESSED;
            break;
    }

    if(detectResult == TOUCH_PRESSED){
        if(XPT2046_GetTouchPoint( &cinfo ) < 0){ //获取触摸坐标
            return detectResult;
        }

        XPT2046_TouchDown( &cinfo );

        cinfo.pre_x = cinfo.x;
        cinfo.pre_y = cinfo.y;
    }
    else {
        // 为负表示不需处理
        if(cinfo.pre_x == -1 && cinfo.pre_x == -1){
            return detectResult;
        }

        //调用触摸被释放时的处理函数，可在该函数编写自己的触摸释放处理过程
        XPT2046_TouchUp( &cinfo );

        //触笔释放，把 xy 重置为负
        cinfo.x = -1;
        cinfo.y = -1;
        cinfo.pre_x = -1;
        cinfo.pre_y = -1;
    }

    return detectResult;
}
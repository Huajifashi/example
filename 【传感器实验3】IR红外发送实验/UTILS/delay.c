#include "delay.h"

void systick_delay_us(uint32_t us){
    uint32_t ticks = 0;
    uint32_t told,tnow,reload,tcnt = 0;

    reload = SysTick->LOAD;           //获取重装载寄存器值
    ticks  = us * (SystemCoreClock / 1000000);  //需要的总计数值
    told   = SysTick->VAL;                   //获取当前数值寄存器值（开始时数值）

    while( 1 ){
        tnow = SysTick->VAL;
        if(tnow == told){//没有差值不需处理
          continue ;
        }

        if(tnow < told){             //当前值小于开始数值，说明未计到0
          tcnt += told - tnow;
        }
        else {//当前值大于开始数值，说明已计到0并重新计数
          tcnt += reload - tnow + told;
        }

        told = tnow;
        if(tcnt >= ticks) { break; }
    }
}
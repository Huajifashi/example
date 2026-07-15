#include "delay.h" 

void delay_us(uint16_t nus){
	uint16_t i;
	while(nus--){
		i = 31;
		while(i--){};
	}
}

void delay_ms(u16 nms){
	uint16_t i;
	while(nms--){
		i = 33800;
		while(i--){};
	}
}

/* 基于 DWT 周期计数器的 μs 延时, 供 ir_send 等模块使用 */
void systick_delay_us(uint32_t nus)
{
	uint32_t start;
	uint32_t cycles = nus * (SystemCoreClock / 1000000);  /* 168 cycles/μs @168MHz */

	/* 使能 DWT 周期计数器 */
	if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk)) {
		CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	}
	if (!(DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk)) {
		DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
	}

	start = DWT->CYCCNT;
	while ((DWT->CYCCNT - start) < cycles);
}

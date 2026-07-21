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

/*****  简单延时函数,ms,us  *****/
void delay_us(uint16_t nus){
    uint16_t i;
    while(nus--){
        i = 31;         //秒表1分钟测试31
        while(i--);
    }
}

void delay_ms(u16 nms){
    uint16_t i;
    while(nms--){
        i = 33800;       //秒表1分钟测试33800
        while(i--);
    }
}
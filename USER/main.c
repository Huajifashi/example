#include <stdio.h>
#include "stm32f4xx.h"
#include "usart.h"
#include "delay.h"
#include "lcd_ili9341.h"
#include "ir_send.h"
#include "ir_recv.h"

/* ========================= GPIO 引脚定义 ========================= */
#define KEY1_PORT     GPIOA
#define KEY1_PIN      GPIO_Pin_0       /* KEY1: PA0 */

#define KEY2_PORT     GPIOC
#define KEY2_PIN      GPIO_Pin_13      /* KEY2: PC13 */

#define KEY3_PORT     GPIOC
#define KEY3_PIN      GPIO_Pin_6       /* KEY3: PC6 */

#define KEY4_PORT     GPIOC
#define KEY4_PIN      GPIO_Pin_7       /* KEY4: PC7 */

#define LED1_PORT     GPIOB
#define LED1_PIN      GPIO_Pin_5       /* LED1: PB5 */

#define LED2_PORT     GPIOB
#define LED2_PIN      GPIO_Pin_0       /* LED2: PB0 */

#define LED3_PORT     GPIOB
#define LED3_PIN      GPIO_Pin_1       /* LED3: PB1 */

#define BUZZER_PORT   GPIOA
#define BUZZER_PIN    GPIO_Pin_8       /* 蜂鸣器: PA8 */

/* ========================= 模式定义 ========================= */
#define MODE_COOL        0             /* 制冷 */
#define MODE_HEAT        1             /* 制热 */
#define MODE_AUTO        2             /* 自动 */
#define MODE_FAN         3             /* 送风 */
#define MODE_DEHUMIDIFY  4             /* 除湿 */

/* ========================= 风速定义 ========================= */
#define FAN_LOW          0             /* 低 */
#define FAN_MID          1             /* 中 */
#define FAN_HIGH         2             /* 高 */
#define FAN_AUTO         3             /* 自动 */

/* ========================= 按键消抖周期 ========================= */
#define DEBOUNCE_MS      20            /* 消抖时间 */
#define LONG_PRESS_MS    1000          /* 长按阈值 1秒 */
#define LED3_BLINK_MS    250           /* LED3 闪烁时长 */

/* ========================= 全局变量 ========================= */
volatile uint32_t g_tick = 0;           /* SysTick 1ms 计数器 */

/* 系统状态 */
uint8_t  g_power     = 0;              /* 0=关机, 1=开机 */
uint8_t  g_mode      = MODE_COOL;      /* 当前模式 */
uint8_t  g_lastMode  = MODE_COOL;      /* 关机前记忆的模式 */
int8_t   g_temp      = 26;             /* 当前温度, 初始26℃ */
uint8_t  g_fan       = FAN_AUTO;       /* 风速 */

/* 按键状态: 0=松开, 1=按下 */
uint8_t  g_keyState[4] = {0, 0, 0, 0};
/* 按键按下时刻(tick) */
uint32_t g_keyPressTick[4] = {0, 0, 0, 0};
/* KEY1长按是否已触发(避免重复触发) */
uint8_t  g_key1LongDone = 0;
/* KEY4长按是否已触发(定时进入/取消) */
uint8_t  g_key4LongDone = 0;

/* 定时相关 */
uint8_t  g_timerSetup    = 0;   /* 0=正常模式, 1=定时设置界面 */
uint8_t  g_timerActive   = 0;   /* 倒计时进行中 */
int16_t  g_timerSec      = 10;  /* 定时秒数 (1~99) */
uint32_t g_timerTick     = 0;   /* 倒计时起始 tick */
uint32_t g_timerLastDisp = 0;   /* 上次显示的已流逝秒数 */

/* LED3 闪烁 */
uint8_t  g_led3On    = 0;
uint32_t g_led3Tick  = 0;

/* 显示刷新标志 */
uint8_t  g_dispDirty = 1;

/* IR 发送标志：1=需要发送红外信号 */
uint8_t  g_irSendFlag = 0;

/* IR 接收多帧组装状态 */
uint8_t  g_ac_ctrl_byte     = 0;
uint8_t  g_ac_temp_byte     = 0;
uint8_t  g_ac_timer_byte    = 0;
uint8_t  g_ac_frame_idx     = 0;     /* 0=等待帧1, 1=等待帧2, 2=等待帧3 */
uint32_t g_ac_last_tick     = 0;     /* 上一帧接收时刻(超时用) */
#define AC_FRAME_TIMEOUT_MS  500      /* 帧间超时 500ms */

/* ========================= 函数声明 ========================= */
static void All_GPIO_Config(void);
static void SysTick_Init(void);
static uint8_t Key_ReadStable(uint8_t id);
static void Buzzer_On(void);
static void Buzzer_Off(void);
static void LED1_Set(uint8_t on);
static void LED3_Trigger(void);
static void LCD_Refresh(void);
static void Key_Scan(void);
static void IR_Send_AC_Signal(void);
void IR_Rece_Proc(uint16_t addr, uint8_t code);

/* ====================== GPIO 初始化 ====================== */
static void All_GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 使能 GPIOA / GPIOB / GPIOC 时钟 */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA |
                           RCC_AHB1Periph_GPIOB |
                           RCC_AHB1Periph_GPIOC, ENABLE);

    /* ---- 按键: 上拉输入 ---- */
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;

    GPIO_InitStructure.GPIO_Pin = KEY1_PIN;
    GPIO_Init(KEY1_PORT, &GPIO_InitStructure);   /* PA0 */

    GPIO_InitStructure.GPIO_Pin = KEY2_PIN;
    GPIO_Init(KEY2_PORT, &GPIO_InitStructure);   /* PC13 */

    GPIO_InitStructure.GPIO_Pin = KEY3_PIN;
    GPIO_Init(KEY3_PORT, &GPIO_InitStructure);   /* PC6 */

    GPIO_InitStructure.GPIO_Pin = KEY4_PIN;
    GPIO_Init(KEY4_PORT, &GPIO_InitStructure);   /* PC7 */

    /* ---- LED & 蜂鸣器: 推挽输出 ---- */
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;

    GPIO_InitStructure.GPIO_Pin = LED1_PIN;
    GPIO_Init(LED1_PORT, &GPIO_InitStructure);   /* PB5 */

    GPIO_InitStructure.GPIO_Pin = LED2_PIN;
    GPIO_Init(LED2_PORT, &GPIO_InitStructure);   /* PB0 */

    GPIO_InitStructure.GPIO_Pin = LED3_PIN;
    GPIO_Init(LED3_PORT, &GPIO_InitStructure);   /* PB1 */

    GPIO_InitStructure.GPIO_Pin = BUZZER_PIN;
    GPIO_Init(BUZZER_PORT, &GPIO_InitStructure); /* PA8 */

    /* 初始状态: 全部关闭 */
    GPIO_SetBits(LED1_PORT, LED1_PIN);
    GPIO_SetBits(LED2_PORT, LED2_PIN);
    GPIO_SetBits(LED3_PORT, LED3_PIN);
    GPIO_SetBits(BUZZER_PORT, BUZZER_PIN);
}

/* ====================== SysTick 1ms ====================== */
static void SysTick_Init(void)
{
    /* 168MHz / 1000 = 168000 → 1ms 中断 */
    if (SysTick_Config(SystemCoreClock / 1000)) {
        while (1);
    }
}

void SysTick_Handler(void)
{
    g_tick++;
}

/* ====================== 带消抖的按键读取 ====================== */
/* 连续 DEBOUNCE_MS/10 次读到相同电平才认为稳定
 * 返回 0=按下, 1=松开 */
static uint8_t Key_ReadStable(uint8_t id)
{
    uint8_t raw, last;
    uint16_t i;
    uint8_t cnt;

    /* 获取 GPIO 端口/引脚 */
    GPIO_TypeDef *port;
    uint16_t pin;
    switch (id) {
        case 0: port = KEY1_PORT; pin = KEY1_PIN; break;
        case 1: port = KEY2_PORT; pin = KEY2_PIN; break;
        case 2: port = KEY3_PORT; pin = KEY3_PIN; break;
        case 3: port = KEY4_PORT; pin = KEY4_PIN; break;
        default: return 1;
    }

    raw = GPIO_ReadInputDataBit(port, pin);
    /* 快速消抖: 连续多次采样一致才返回 */
    cnt = 1;
    for (i = 0; i < (DEBOUNCE_MS / 5); i++) {
        delay_us(5000);  /* 5ms */
        last = raw;
        raw = GPIO_ReadInputDataBit(port, pin);
        if (raw == last) {
            cnt++;
            if (cnt >= 3) return raw;  /* 连续3次一致 → 稳定 */
        } else {
            cnt = 1;
        }
    }
    return raw;
}

/* ====================== 蜂鸣器 ====================== */
static void Buzzer_On(void)
{
    GPIO_ResetBits(BUZZER_PORT, BUZZER_PIN);
}

static void Buzzer_Off(void)
{
    GPIO_SetBits(BUZZER_PORT, BUZZER_PIN);
}

/* ====================== LED 控制 ====================== */
static void LED1_Set(uint8_t on)
{
    if (on)
        GPIO_ResetBits(LED1_PORT, LED1_PIN);
    else
        GPIO_SetBits(LED1_PORT, LED1_PIN);
}

/* 触发 LED3 亮起 250ms (主循环中处理熄灭) */
static void LED3_Trigger(void)
{
    GPIO_ResetBits(LED3_PORT, LED3_PIN);
    g_led3On   = 1;
    g_led3Tick = g_tick;
}

/* ====================== LCD 刷新显示 ====================== */
static void LCD_Refresh(void)
{
    char buf[32];
    const char *modeStr;
    const char *fanStr;

    /* 全屏清黑底 */
    LCD_SetTextColor(WHITE);
    LCD_SetBackColor(BLACK);
    LCD_Clear(0, 0, LCD_GetLenX(), LCD_GetLenY());

    /* ============ 定时设置界面 ============ */
    if (g_timerSetup) {
        LCD_SetTextColor(YELLOW);
        LCD_SetBackColor(BLACK);
        LCD_DispStringEN(10, 20, 0, "Timer Setup");

        LCD_SetTextColor(CYAN);
        LCD_SetBackColor(BLACK);
        LCD_DispStringEN(10, 70, 0, "Time:");

        sprintf(buf, "%d s          ", g_timerSec);
        LCD_SetTextColor(WHITE);
        LCD_SetBackColor(BLACK);
        LCD_DispStringEN(80, 70, 0, buf);

        LCD_SetTextColor(CYAN);
        LCD_SetBackColor(BLACK);
        LCD_DispStringEN(10, 120, 0, "KEY4: OK");
        LCD_DispStringEN(10, 150, 0, "Hold: Cancel");

        g_dispDirty = 0;
        return;
    }

    /* ============ 关机画面 ============ */
    if (g_power == 0) {
        LCD_SetTextColor(WHITE);
        LCD_SetBackColor(BLACK);
        LCD_DispStringEN(60, 100, 0, "Power OFF");

        /* 定时倒计时中 */
        if (g_timerActive) {
            int16_t remain = g_timerSec - (int16_t)((g_tick - g_timerTick) / 1000);
            if (remain < 0) remain = 0;
            sprintf(buf, "Timer: %d s  ", remain);
            LCD_SetTextColor(YELLOW);
            LCD_SetBackColor(BLACK);
            LCD_DispStringEN(60, 140, 0, buf);
        }

        g_dispDirty = 0;
        return;
    }

    /* ============ 开机画面 ============ */
    /* 第1行: 模式 */
    LCD_SetTextColor(CYAN);
    LCD_SetBackColor(BLACK);
    LCD_DispStringEN(10, 20, 0, "Mode:");

    switch (g_mode) {
        case MODE_COOL:       modeStr = "Cool       "; break;
        case MODE_HEAT:       modeStr = "Heat       "; break;
        case MODE_AUTO:       modeStr = "Auto       "; break;
        case MODE_FAN:        modeStr = "Fan        "; break;
        case MODE_DEHUMIDIFY: modeStr = "Dehumidify "; break;
        default:              modeStr = "Unknown    "; break;
    }
    LCD_SetTextColor(WHITE);
    LCD_SetBackColor(BLACK);
    LCD_DispStringEN(80, 20, 0, (char *)modeStr);

    /* 第2行: 温度 */
    LCD_SetTextColor(CYAN);
    LCD_SetBackColor(BLACK);
    LCD_DispStringEN(10, 60, 0, "Temp:");

    if (g_mode == MODE_AUTO) {
        LCD_SetTextColor(YELLOW);
        LCD_SetBackColor(BLACK);
        LCD_DispStringEN(80, 60, 0, "Auto       ");
    } else {
        sprintf(buf, "%d C        ", g_temp);
        LCD_SetTextColor(WHITE);
        LCD_SetBackColor(BLACK);
        LCD_DispStringEN(80, 60, 0, buf);
    }

    /* 第3行: 风速 */
    LCD_SetTextColor(CYAN);
    LCD_SetBackColor(BLACK);
    LCD_DispStringEN(10, 100, 0, "Fan:");

    switch (g_fan) {
        case FAN_LOW:  fanStr = "Low        "; break;
        case FAN_MID:  fanStr = "Mid        "; break;
        case FAN_HIGH: fanStr = "High       "; break;
        case FAN_AUTO: fanStr = "Auto       "; break;
        default:       fanStr = "Unknown    "; break;
    }
    LCD_SetTextColor(WHITE);
    LCD_SetBackColor(BLACK);
    LCD_DispStringEN(80, 100, 0, (char *)fanStr);

    /* 定时倒计时中 */
    if (g_timerActive) {
        int16_t remain = g_timerSec - (int16_t)((g_tick - g_timerTick) / 1000);
        if (remain < 0) remain = 0;
        sprintf(buf, "Timer: %d s  ", remain);
        LCD_SetTextColor(YELLOW);
        LCD_SetBackColor(BLACK);
        LCD_DispStringEN(10, 140, 0, buf);
    }

    g_dispDirty = 0;
}

/* ====================== 按键扫描 & 逻辑处理 ====================== */
static void Key_Scan(void)
{
    uint8_t  k[4];
    uint8_t  anyDown = 0;

    /* 读取所有按键稳定值 */
    k[0] = Key_ReadStable(0);  /* KEY1 */
    k[1] = Key_ReadStable(1);  /* KEY2 */
    k[2] = Key_ReadStable(2);  /* KEY3 */
    k[3] = Key_ReadStable(3);  /* KEY4 */

    uint32_t now = g_tick;

    /* ============ 定时设置界面 ============ */
    if (g_timerSetup) {

        /* KEY1: 在定时界面不处理 */

        /* KEY2: 减1秒 (按下即触发) */
        if (k[1] == 0) {
            anyDown = 1;
            if (g_keyState[1] == 0) {
                g_keyState[1] = 1;
                if (g_timerSec > 1) {
                    g_timerSec--;
                    g_dispDirty = 1;
                    LED3_Trigger();
                }
            }
        } else {
            g_keyState[1] = 0;
        }

        /* KEY3: 加1秒 (按下即触发) */
        if (k[2] == 0) {
            anyDown = 1;
            if (g_keyState[2] == 0) {
                g_keyState[2] = 1;
                if (g_timerSec < 99) {
                    g_timerSec++;
                    g_dispDirty = 1;
                    LED3_Trigger();
                }
            }
        } else {
            g_keyState[2] = 0;
        }

        /* KEY4: 短按确认 / 长按取消 */
        if (k[3] == 0) {
            anyDown = 1;
            if (g_keyState[3] == 0) {
                g_keyState[3]     = 1;
                g_keyPressTick[3] = now;
                g_key4LongDone    = 0;
            }
            if (!g_key4LongDone && (now - g_keyPressTick[3] >= LONG_PRESS_MS)) {
                g_key4LongDone = 1;
                /* 取消定时, 回到正常界面 */
                g_timerSetup  = 0;
                g_timerActive = 0;
                g_dispDirty   = 1;
                LED3_Trigger();
                g_irSendFlag  = 1;   /* 中止定时 → 发送 IR */
            }
        } else {
            if (g_keyState[3] == 1 && !g_key4LongDone) {
                /* 确认定时: 记录当前电量状态, 开始倒计时 */
                g_timerSetup  = 0;
                g_timerActive = 1;
                g_timerTick   = now;
                g_timerLastDisp = 0;
                g_dispDirty   = 1;
                LED3_Trigger();
                g_irSendFlag  = 1;   /* 完成定时设定 → 发送 IR */
            }
            g_keyState[3] = 0;
        }

        /* 蜂鸣器 */
        if (anyDown) Buzzer_On(); else Buzzer_Off();

        /* LED3 超时 */
        if (g_led3On && (now - g_led3Tick >= LED3_BLINK_MS)) {
            GPIO_SetBits(LED3_PORT, LED3_PIN);
            g_led3On = 0;
        }
        return;
    }

    /* ============ 正常模式 ============ */

    /* ---- KEY1: 长按≥1秒开关机 / 短按调风速 ---- */
    if (k[0] == 0) {
        anyDown = 1;
        if (g_keyState[0] == 0) {
            g_keyState[0]     = 1;
            g_keyPressTick[0] = now;
            g_key1LongDone    = 0;
        }
        if (!g_key1LongDone && (now - g_keyPressTick[0] >= LONG_PRESS_MS)) {
            g_key1LongDone = 1;
            g_power = !g_power;
            if (g_power) {
                LED1_Set(1);
                g_mode = g_lastMode;
            } else {
                LED1_Set(0);
            }
            /* 开关机时取消正在运行的定时 */
            g_timerActive = 0;
            LED3_Trigger();
            g_dispDirty = 1;
            g_irSendFlag = 1;
        }
    } else {
        if (g_keyState[0] == 1 && !g_key1LongDone) {
            if (g_power) {
                g_fan = (g_fan + 1) % 4;
                LED3_Trigger();
                g_dispDirty = 1;
                g_irSendFlag = 1;
            }
        }
        g_keyState[0] = 0;
    }

    /* ---- KEY2: 温度- ---- */
    if (k[1] == 0) {
        anyDown = 1;
        if (g_keyState[1] == 0) {
            g_keyState[1]     = 1;
            g_keyPressTick[1] = now;
        }
    } else {
        if (g_keyState[1] == 1) {
            if (g_power && g_mode != MODE_AUTO && g_temp > 16) {
                g_temp--;
                LED3_Trigger();
                g_dispDirty = 1;
                g_irSendFlag = 1;
            }
            g_keyState[1] = 0;
        }
    }

    /* ---- KEY3: 温度+ ---- */
    if (k[2] == 0) {
        anyDown = 1;
        if (g_keyState[2] == 0) {
            g_keyState[2]     = 1;
            g_keyPressTick[2] = now;
        }
    } else {
        if (g_keyState[2] == 1) {
            if (g_power && g_mode != MODE_AUTO && g_temp < 30) {
                g_temp++;
                LED3_Trigger();
                g_dispDirty = 1;
                g_irSendFlag = 1;
            }
            g_keyState[2] = 0;
        }
    }

    /* ---- KEY4: 长按进入定时 / 短按模式切换 ---- */
    if (k[3] == 0) {
        anyDown = 1;
        if (g_keyState[3] == 0) {
            g_keyState[3]     = 1;
            g_keyPressTick[3] = now;
            g_key4LongDone    = 0;
        }
        if (!g_key4LongDone && (now - g_keyPressTick[3] >= LONG_PRESS_MS)) {
            g_key4LongDone = 1;
            /* 进入定时设置 */
            g_timerSetup    = 1;
            g_timerActive   = 0;
            g_timerSec      = 10;
            g_keyState[1]   = 0;  /* 清除 KEY2/3 旧状态 */
            g_keyState[2]   = 0;
            g_dispDirty     = 1;
            LED3_Trigger();
        }
    } else {
        if (g_keyState[3] == 1 && !g_key4LongDone) {
            /* 短按: 模式切换 */
            if (g_power) {
                g_mode++;
                if (g_mode > MODE_DEHUMIDIFY) g_mode = MODE_COOL;
                g_lastMode = g_mode;
                LED3_Trigger();
                g_dispDirty = 1;
                g_irSendFlag = 1;
            }
        }
        g_keyState[3] = 0;
    }

    /* ---- 蜂鸣器 ---- */
    if (anyDown) Buzzer_On(); else Buzzer_Off();

    /* ---- LED3 超时熄灭 ---- */
    if (g_led3On && (now - g_led3Tick >= LED3_BLINK_MS)) {
        GPIO_SetBits(LED3_PORT, LED3_PIN);
        g_led3On = 0;
    }
}

/* ====================== 红外接收回调（覆盖弱函数） ====================== */
void IR_Rece_Proc(uint16_t addr, uint8_t code)
{
    uint32_t now = g_tick;
    const char *modeStr[] = {"Cool", "Heat", "Auto", "Fan", "Dehumidify"};
    const char *fanStr[]  = {"Low", "Mid", "High", "Auto"};

    /* 超时复位：>500ms 无下一帧则丢弃旧状态 */
    if (g_ac_frame_idx > 0 && (now - g_ac_last_tick > AC_FRAME_TIMEOUT_MS)) {
        g_ac_frame_idx = 0;
    }

    switch (addr) {
        case 0xA5: /* 帧1: 控制字节 */
            g_ac_ctrl_byte = code;
            g_ac_frame_idx = 1;
            g_ac_last_tick = now;
            break;

        case 0xA6: /* 帧2: 温度 */
            if (g_ac_frame_idx == 1) {
                g_ac_temp_byte = code;
                g_ac_frame_idx = 2;
                g_ac_last_tick = now;
            } else {
                g_ac_frame_idx = 0;
                printf("IR Decoded: Failed (seq err at Temp frame)\r\n");
            }
            break;

        case 0xA7: /* 帧3: 定时秒数 */
            if (g_ac_frame_idx == 2) {
                g_ac_timer_byte = code;
                g_ac_frame_idx = 0;  /* 完整一帧 */

                /* 解码 */
                uint8_t power   = (g_ac_ctrl_byte >> 7) & 0x01;
                uint8_t mode    = (g_ac_ctrl_byte >> 4) & 0x07;
                uint8_t fan     = (g_ac_ctrl_byte >> 2) & 0x03;
                uint8_t tmr     = (g_ac_ctrl_byte >> 1) & 0x01;
                uint8_t temp    = g_ac_temp_byte;
                uint8_t tmr_sec = g_ac_timer_byte;

                printf("IR Decoded: Power=%s, Temp=%dC, Mode=%s, Fan=%s, Timer=%s(%ds)\r\n",
                       power  ? "ON"  : "OFF",
                       temp,
                       mode < 5   ? modeStr[mode] : "Unknown",
                       fan  < 4   ? fanStr[fan]   : "Unknown",
                       tmr   ? "ON"  : "OFF",
                       tmr_sec);
            } else {
                g_ac_frame_idx = 0;
                printf("IR Decoded: Failed (seq err at Timer frame)\r\n");
            }
            break;

        default:
            /* 未知地址 → 复位并报失败 */
            g_ac_frame_idx = 0;
            printf("IR Decoded: Failed (unknown addr 0x%02X)\r\n", addr);
            break;
    }
}

/* ====================== 红外发送（空调协议） ====================== */
static void IR_Send_AC_Signal(void)
{
    uint8_t ctrl, temp_byte, timer_byte;

    /* 构建控制字节：
     * Bit7      : 电源 0=关/1=开
     * Bit6-4    : 模式 0~4
     * Bit3-2    : 风速 0~3
     * Bit1      : 定时 0=无/1=有
     * Bit0      : 保留=0
     */
    ctrl      = (g_power << 7)
              | ((g_mode & 0x07) << 4)
              | ((g_fan  & 0x03) << 2)
              | ((g_timerActive ? 1 : 0) << 1);
    temp_byte = (uint8_t)g_temp;
    timer_byte = g_timerActive ? (uint8_t)g_timerSec : 0;

    /* 帧1: 控制字节 */
    IR_Send(0xA5, ctrl);
    delay_ms(20);
    IR_Recv();   /* 处理接收到的帧1 */

    /* 帧2: 温度 */
    IR_Send(0xA6, temp_byte);
    delay_ms(20);
    IR_Recv();

    /* 帧3: 定时秒数 */
    IR_Send(0xA7, timer_byte);
    delay_ms(20);
    IR_Recv();
}

/* ====================== 主函数 ====================== */
int main(void)
{
    /* 系统初始化 */
    SystemInit();

    /* SysTick 1ms */
    SysTick_Init();

    /* GPIO 初始化 (按键/LED/蜂鸣器) */
    All_GPIO_Config();

    /* 串口初始化 (调试用) */
    UART2_Init(115200);

    /* LCD 初始化 */
    LCD_Init();

    /* 红外发送初始化 */
    IR_Send_Init();

    /* 红外接收初始化 */
    IR_Recv_Init();

    /* ---- 启动画面 ---- */
    LCD_SetTextColor(CYAN);
    LCD_SetBackColor(BLACK);
    LCD_Clear(0, 0, LCD_GetLenX(), LCD_GetLenY());
    LCD_SetTextColor(WHITE);
    LCD_SetBackColor(BLACK);
    LCD_DispStringEN(40, 100, 0, "AC Remote");
    LCD_DispStringEN(20, 140, 0, "Initializing...");
    delay_ms(1000);

    /* 初始显示: 关机状态 */
    g_dispDirty = 1;

    while (1) {
        /* 按键扫描 & 逻辑处理 */
        Key_Scan();

        /* IR 发送标志处理 */
        if (g_irSendFlag) {
            g_irSendFlag = 0;
            IR_Send_AC_Signal();
        }

        /* 定时倒计时检查 */
        if (g_timerActive) {
            uint32_t elapsed = (g_tick - g_timerTick) / 1000;
            if (elapsed >= (uint32_t)g_timerSec) {
                /* 时间到，执行开关机 */
                g_timerActive = 0;
                g_power = !g_power;
                if (g_power) {
                    LED1_Set(1);
                    g_mode = g_lastMode;
                } else {
                    LED1_Set(0);
                }
                LED3_Trigger();
                g_dispDirty = 1;
                g_irSendFlag = 1;   /* 定时完成 → 发送 IR */
            } else if (elapsed != g_timerLastDisp) {
                /* 剩余秒数变化，刷新显示 */
                g_timerLastDisp = elapsed;
                g_dispDirty = 1;
            }
        }

        /* 红外接收轮询（持续监听） */
        IR_Recv();

        /* 需要刷新 LCD 时刷新 */
        if (g_dispDirty) {
            LCD_Refresh();
        }

        /* LED2: 有定时任务时亮起 */
        if (g_timerActive || g_timerSetup) {
            GPIO_ResetBits(LED2_PORT, LED2_PIN);
        } else {
            GPIO_SetBits(LED2_PORT, LED2_PIN);
        }

        delay_ms(10);  /* 主循环周期 ~10ms */
    }
}

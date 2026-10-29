#ifndef __GPIO_H__
#define __GPIO_H__

#include <stdint.h>
#include "cw32f030_gpio.h"

/* 原理图：K1/PB12、K2/PB13、K3/PB14，另一端接地，按下为低电平。 */
#define KEY_EVENT_MODE 0x01U
#define KEY_EVENT_SAVE 0x02U
#define KEY_EVENT_EXIT 0x04U
#define KEY_PRESS_MS   100U /* 连续按下100ms确认，长按只触发一次 */
#define KEY_RELEASE_MS 20U  /* 连续松开20ms后允许下一次触发 */

void LED_Init(void);
void KEY_Init(void);
void Key_Scan(void);       /* 仅由1ms定时器中断调用，禁止在主循环重复调用 */
uint8_t KEY_GetEvents(void); /* 主循环原子读取并清空事件，不在中断里擦写Flash */

#define LED_ON  GPIO_WritePin(CW_GPIOC,GPIO_PIN_13,GPIO_Pin_RESET)
#define LED_OFF GPIO_WritePin(CW_GPIOC,GPIO_PIN_13,GPIO_Pin_SET)

#endif

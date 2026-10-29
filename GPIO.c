#include "main.h"
#include "GPIO.h"

static volatile uint8_t KeyEvents = 0;
static uint8_t PressCount[3] = {0};
static uint8_t ReleaseCount[3] = {0};
static uint8_t Held[3] = {0};

void LED_Init(void)
{
  GPIO_InitTypeDef init;
  __RCC_GPIOC_CLK_ENABLE();
  init.IT = GPIO_IT_NONE;
  init.Mode = GPIO_MODE_OUTPUT_PP;
  init.Pins = GPIO_PIN_13;
  init.Speed = GPIO_SPEED_HIGH;
  GPIO_Init(CW_GPIOC, &init);
  LED_OFF; /* PC13低电平点亮，默认关闭。 */
}

void KEY_Init(void)
{
  GPIO_InitTypeDef init;
  uint8_t i;
  __RCC_GPIOB_CLK_ENABLE();
  PB12_DIGTAL_ENABLE();
  PB13_DIGTAL_ENABLE();
  PB14_DIGTAL_ENABLE();
  init.Pins = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14;
  init.Mode = GPIO_MODE_INPUT_PULLUP;
  init.IT = GPIO_IT_NONE;
  init.Speed = GPIO_SPEED_HIGH;
  GPIO_Init(CW_GPIOB, &init);
  KeyEvents = 0;
  for (i = 0; i < 3; i++) PressCount[i] = ReleaseCount[i] = Held[i] = 0;
}

/* 每1ms执行一次：按下消抖、松开消抖、长按锁定。
 * 只置事件位，不在中断里计算系数、格式化显示或擦写Flash。
 * 这避免参考程序每按住100ms就重复切模式/反复擦写的问题。
 */
void Key_Scan(void)
{
  static const uint16_t pins[3] = {GPIO_PIN_12, GPIO_PIN_13, GPIO_PIN_14};
  uint8_t i;
  for (i = 0; i < 3; i++)
  {
    if (GPIO_ReadPin(CW_GPIOB, pins[i]) == GPIO_Pin_RESET)
    {
      ReleaseCount[i] = 0;
      if (!Held[i] && ++PressCount[i] >= KEY_PRESS_MS)
      {
        Held[i] = 1;
        PressCount[i] = 0;
        KeyEvents |= (uint8_t)(1U << i);
      }
    }
    else
    {
      PressCount[i] = 0;
      if (Held[i] && ++ReleaseCount[i] >= KEY_RELEASE_MS)
      {
        Held[i] = 0;
        ReleaseCount[i] = 0;
      }
    }
  }
}

uint8_t KEY_GetEvents(void)
{
  uint32_t primask = __get_PRIMASK();
  uint8_t events;
  __disable_irq();
  events = KeyEvents;
  KeyEvents = 0;
  __set_PRIMASK(primask);
  return events;
}

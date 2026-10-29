#include "main.h"
#include "TIM.h"
#include "Seg_Display.h"
#include "ADC.h"
#include "GPIO.h"

void Btim1_Init(void)
{
  BTIM_TimeBaseInitTypeDef BTIM_TimeBaseInitStruct;
  __RCC_BTIM_CLK_ENABLE();

  /* 外设时钟64MHz、不分频：64000个计数为1ms。 */
  BTIM_TimeBaseInitStruct.BTIM_Mode      = BTIM_Mode_TIMER;
  BTIM_TimeBaseInitStruct.BTIM_Period    = 64000U - 1U;
  BTIM_TimeBaseInitStruct.BTIM_Prescaler = BTIM_PRS_DIV1;
  BTIM_TimeBaseInitStruct.BTIM_OPMode    = BTIM_OPMode_Repetitive;

  BTIM_TimeBaseInit(CW_BTIM1, &BTIM_TimeBaseInitStruct);
  BTIM_ClearITPendingBit(CW_BTIM1, BTIM_IT_OV);
  NVIC_ClearPendingIRQ(BTIM1_IRQn);
  NVIC_EnableIRQ(BTIM1_IRQn);
  BTIM_ITConfig(CW_BTIM1, BTIM_IT_OV, ENABLE);
  BTIM_Cmd(CW_BTIM1, ENABLE);
}

void BTIM1_IRQHandler(void)
{

  if (BTIM_GetITStatus(CW_BTIM1, BTIM_IT_OV))
  {
    BTIM_ClearITPendingBit(CW_BTIM1, BTIM_IT_OV);
    Get_ADC_Value(); /* 采样两路，只保存原始码。 */
    Key_Scan();      /* 按键只产生事件，主循环处理标定与Flash。 */
    Display_Refresh(); /* 每 1 ms 扫描一位，六位完整扫描周期为 6 ms。 */
  }
}

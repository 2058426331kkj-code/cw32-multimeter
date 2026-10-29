#include "main.h"
#include "GPIO.h"
#include "ADC.h"
#include "TIM.h"
#include "Seg_Display.h"
#include "Calibration.h"

int main(void)
{
  uint32_t Led_Dis_Time;
  uint32_t now;
  uint8_t redraw;

  RCC_Configuration(); /* 系统64MHz，同时初始化SysTick毫秒计时。 */
  Seg_Init();
  Close_Com();
  KEY_Init();          /* K1=PB12、K2=PB13、K3=PB14，低电平有效。 */
  LED_Init();
  read_vol_cur_calibration(); /* 上电读取两页中最新的有效记录，并计算系数。 */
  DisplayBuff();       /* 模式0：上排电压、下排电流，初始值均为0。 */
  ADC1_Init();
  Btim1_Init();        /* ADC初始化完成后再启动1ms采样、按键及扫描中断。 */
  Led_Dis_Time = GetTick();

  while (1)
  {
    /* 标定保存只能在主循环执行；成功返回模式0，失败保留模式并显示E码。 */
    redraw = Calibration_Process();
    now = GetTick();
    /* 无符号减法可处理32位计时溢出；300ms仅控制数值更新，不停止扫描。 */
    if ((uint32_t)(now - Led_Dis_Time) >= 300U)
    {
      Led_Dis_Time = now;
      Volt_Cal();
      redraw = 1;
    }
    if (redraw) DisplayBuff();
  }
}

void RCC_Configuration(void)
{
  RCC_HSI_Enable(RCC_HSIOSC_DIV6);

  /* 1. 设置HCLK和PCLK的分频系数 */
  RCC_HCLKPRS_Config(RCC_HCLK_DIV1);
  RCC_PCLKPRS_Config(RCC_PCLK_DIV1);

  /* 2. 使能PLL，通过HSI倍频到64MHz */
  RCC_PLL_Enable(RCC_PLLSOURCE_HSI, 8000000, 8);     
  // PLL输出频率64MHz

  /*< 当使用的时钟源HCLK大于24M,小于等于48MHz：设置FLASH 读等待周期为2 cycle
  < 当使用的时钟源HCLK大于48M,小于等于72MHz：设置FLASH 读等待周期为3 cycle */    
  __RCC_FLASH_CLK_ENABLE();
  FLASH_SetLatency(FLASH_Latency_3);

  /* 3. 时钟切换到PLL */
  RCC_SysClk_Switch(RCC_SYSCLKSRC_PLL);
  RCC_SystemCoreClockUpdate(64000000);
  InitTick(64000000); /* 建立1ms系统计时，用于每300ms更新测量值。 */
}

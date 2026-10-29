#include "main.h"
#include "ADC.h"
#include "Calibration.h"

volatile uint16_t Volt_Buffer[ADC_SAMPLE_SIZE] = {0};
volatile uint16_t Curr_Buffer[ADC_SAMPLE_SIZE] = {0};
static volatile uint8_t Adc_Sample_Count = 0;
uint32_t V_Buffer = 0;
uint32_t I_Buffer = 0;

void ADC1_Init(void)
{
  ADC_InitTypeDef ADC_InitStructure;
  ADC_SerialChTypeDef ADC_SerialChStructure = {0};
  GPIO_InitTypeDef GPIO_Init_Struct;

  __RCC_GPIOB_CLK_ENABLE();
  __RCC_ADC_CLK_ENABLE();
  GPIO_Init_Struct.IT = GPIO_IT_NONE;
  GPIO_Init_Struct.Mode = GPIO_MODE_ANALOG;
  GPIO_Init_Struct.Pins = GPIO_PIN_10 | GPIO_PIN_11;
  GPIO_Init_Struct.Speed = GPIO_SPEED_HIGH;
  GPIO_Init(CW_GPIOB, &GPIO_Init_Struct);
  PB10_ANALOG_ENABLE(); /* PB10 = ADC IN11，电压通道 */
  PB11_ANALOG_ENABLE(); /* PB11 = ADC IN12，电流通道 */

  ADC_StructInit(&ADC_InitStructure);
  ADC_InitStructure.ADC_ClkDiv = ADC_Clk_Div128; /* 64 MHz / 128 = 500 kHz */
  ADC_InitStructure.ADC_VrefSel = ADC_Vref_BGR1p5;
  ADC_InitStructure.ADC_SampleTime = ADC_SampTime10Clk;

  ADC_SerialChStructure.ADC_Sqr0Chmux = ADC_SqrCh11;
  ADC_SerialChStructure.ADC_Sqr1Chmux = ADC_SqrCh12;
  ADC_SerialChStructure.ADC_SqrEns = ADC_SqrEns01;
  ADC_SerialChStructure.ADC_InitStruct = ADC_InitStructure;
  ADC_SerialChContinuousModeCfg(&ADC_SerialChStructure);
  ADC_ClearITPendingAll();
  ADC_Enable();
  ADC_SoftwareStartConvCmd(ENABLE);
}

/* BTIM1 每 1 ms 调用，两路使用相同的环形缓冲下标。 */
void Get_ADC_Value(void)
{
  static uint8_t adc_cnt = 0;
  uint16_t voltage;
  uint16_t current;

  ADC_GetSqr0Result(&voltage);
  ADC_GetSqr1Result(&current);
  Volt_Buffer[adc_cnt] = voltage;
  Curr_Buffer[adc_cnt] = current;
  adc_cnt++;
  if (adc_cnt >= ADC_SAMPLE_SIZE)
  {
    adc_cnt = 0;
  }
  if (Adc_Sample_Count < ADC_SAMPLE_SIZE)
  {
    Adc_Sample_Count++;
  }
}

/* 一起复制两路数据；临界区只复制，滤波计算放在恢复中断之后。 */
uint8_t ADC_CopySamples(uint16_t volts[ADC_SAMPLE_SIZE],
                        uint16_t currents[ADC_SAMPLE_SIZE])
{
  uint32_t primask;
  uint32_t i;

  if ((volts == 0) || (currents == 0) || (Adc_Sample_Count < ADC_SAMPLE_SIZE))
  {
    return 0;
  }
  primask = __get_PRIMASK();
  __disable_irq();
  for (i = 0; i < ADC_SAMPLE_SIZE; i++)
  {
    volts[i] = Volt_Buffer[i];
    currents[i] = Curr_Buffer[i];
  }
  __set_PRIMASK(primask);
  return 1;
}

uint32_t Mean_Value_Filter(const uint16_t *value, uint32_t size)
{
  uint32_t sum = 0;
  uint16_t max = 0;
  uint16_t min = 0xffff;
  uint32_t i;

  if ((value == 0) || (size == 0))
  {
    return 0;
  }

  for (i = 0; i < size; i++)
  {
    sum += value[i];
    if (value[i] > max)
    {
      max = value[i];
    }
    if (value[i] < min)
    {
      min = value[i];
    }
  }

  if (size <= 2)
  {
    return sum / size;
  }
  return (sum - max - min) / (size - 2);
}


/* 主循环每 300 ms 调用；V_Buffer/I_Buffer 均为实际物理量的 100 倍。 */
void Volt_Cal(void)
{
  uint16_t volts[ADC_SAMPLE_SIZE];
  uint16_t currents[ADC_SAMPLE_SIZE];
  uint32_t voltage_adc;
  uint32_t current_adc;

  if (!ADC_CopySamples(volts, currents))
  {
    return;
  }
  voltage_adc = Mean_Value_Filter(volts, ADC_SAMPLE_SIZE);
  current_adc = Mean_Value_Filter(currents, ADC_SAMPLE_SIZE);

  /* 两路滤波后的原始码交给标定模型；无完整标定点时采用理论系数。
   * 输出单位保持0.01V和0.01A，显示层无需再做分压或电阻换算。 */
  V_Buffer = Calibration_Voltage(voltage_adc);
  I_Buffer = Calibration_Current(current_adc);
}

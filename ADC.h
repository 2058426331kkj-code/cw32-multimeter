#ifndef __ADC_H__
#define __ADC_H__

#include <stdint.h>

#define ADC_SAMPLE_SIZE 20U
#define ADC_REF_VALUE   1500UL /* 内部参考电压，单位 mV */
#define R1              10UL
#define R2              220UL
#define CURRENT_SHUNT_MOHM 100UL /* 0.1 欧采样电阻；假定无放大增益 */

extern volatile uint16_t Volt_Buffer[ADC_SAMPLE_SIZE];
extern volatile uint16_t Curr_Buffer[ADC_SAMPLE_SIZE];
extern uint32_t V_Buffer; /* 分压前电压，单位 0.01 V，1234 表示 12.34 V */
extern uint32_t I_Buffer; /* 电流，单位 0.01 A，125 表示 1.25 A */

void ADC1_Init(void);
void Volt_Cal(void);
void Get_ADC_Value(void);
uint8_t ADC_CopySamples(uint16_t volts[ADC_SAMPLE_SIZE],
                        uint16_t currents[ADC_SAMPLE_SIZE]);
uint32_t Mean_Value_Filter(const uint16_t *value, uint32_t size);

#endif

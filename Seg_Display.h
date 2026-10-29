#ifndef __SEG_DISPLAY_H__
#define __SEG_DISPLAY_H__

#include <stdint.h>

void Seg_Dis(uint8_t Pos,uint8_t Num);
void Seg_Init(void);
void Close_Com(void);
void Dynamic_Display(uint32_t value);
void Display(uint32_t value);
void DisplayI(uint32_t value);
void Display_Refresh(void);
/* mode=0正常，1..4标定；error=0正常，1..3错误提示。 */
void Display_Mode(uint8_t mode, uint32_t voltage, uint32_t current, uint8_t error);


#endif

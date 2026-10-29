#include "main.h"
#include "Seg_Display.h"
#include "GPIO.h"
#include "TIM.h"

/*  共阴数码管编码表：
 0x3f   0x06   0x5b  0x4f  0x66  0x6d  0x7d  0x07  0x7f  0x6f
  0      1      2     3     4     5     6     7     8     9
 0xbf   0x86   0xdb  0xcf  0xe6  0xed  0xfd  0x87  0xff  0xef
  0.     1.     2.    3.    4.    5.    6.    7.    8.    9. */
  //0x3f,  0011 1111; 0x5b 0101 1011
  const uint8_t Seg_Table[25] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f,
                           0xbf, 0x86, 0xdb, 0xcf, 0xe6, 0xed, 0xfd, 0x87, 0xff, 0xef,
                           0x6d, 0x77, 0x79, 0x50, 0x00}; /* 20:S 21:A 22:E 23:r 24:空白 */

void Seg_Init(void)  //查找原理图对数码管相关引脚进行初始化
{
	GPIO_InitTypeDef GPIO_InitStruct;
  __RCC_GPIOA_CLK_ENABLE();   //打开GPIOA的时钟
  __RCC_GPIOB_CLK_ENABLE();   //打开GPIOB的时钟
  GPIO_InitStruct.Pins = GPIO_PIN_2 | GPIO_PIN_0 | GPIO_PIN_4 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_1 | GPIO_PIN_3 | GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_15;
  //A:PA02;B:PA00;C:PA04;D:PA06;E:PA07;F:PA01;G:PA03;DP:PA05;
  //COM1:PA08;COM2:PA11;COM3:PA12;COM4:PA15;COM5:PB03;COM6:PB04;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.IT = GPIO_IT_NONE;
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
  GPIO_Init(CW_GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pins = GPIO_PIN_3 | GPIO_PIN_4; //COM5:PB03;COM6:PB04
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.IT = GPIO_IT_NONE;
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
  GPIO_Init(CW_GPIOB, &GPIO_InitStruct);
}

void Seg_Dis(uint8_t Pos,uint8_t Num)
{ 
	int i;
  uint8_t Dis_Value;
  Close_Com();
  if ((Pos < 1U) || (Pos > 6U) || (Num >= 25U)) return;
  Dis_Value = Seg_Table[Num];

  for(i = 0; i < 8; i++)
  {
    switch(i)
    {
      case 0:
      GPIO_WritePin(CW_GPIOA,GPIO_PIN_2,((Dis_Value >> i) & 0x01) ? GPIO_Pin_SET : GPIO_Pin_RESET);    //PA02,A
      break;
      case 1:
      GPIO_WritePin(CW_GPIOA,GPIO_PIN_0,((Dis_Value >> i) & 0x01) ? GPIO_Pin_SET : GPIO_Pin_RESET);    //PA00,B
      break;
      case 2:
      GPIO_WritePin(CW_GPIOA,GPIO_PIN_4,((Dis_Value >> i) & 0x01) ? GPIO_Pin_SET : GPIO_Pin_RESET);    //PA04,C
      break;
      case 3:
      GPIO_WritePin(CW_GPIOA,GPIO_PIN_6,((Dis_Value >> i) & 0x01) ? GPIO_Pin_SET : GPIO_Pin_RESET);    //PA06,D
      break;
      case 4:
      GPIO_WritePin(CW_GPIOA,GPIO_PIN_7,((Dis_Value >> i) & 0x01) ? GPIO_Pin_SET : GPIO_Pin_RESET);    //PA07,E
      break;
      case 5:
      GPIO_WritePin(CW_GPIOA,GPIO_PIN_1,((Dis_Value >> i) & 0x01) ? GPIO_Pin_SET : GPIO_Pin_RESET);    //PA01,F
      break;
      case 6:
      GPIO_WritePin(CW_GPIOA,GPIO_PIN_3,((Dis_Value >> i) & 0x01) ? GPIO_Pin_SET : GPIO_Pin_RESET);    //PA03,G
      break;
      case 7:
      GPIO_WritePin(CW_GPIOA,GPIO_PIN_5,((Dis_Value >> i) & 0x01) ? GPIO_Pin_SET : GPIO_Pin_RESET);    //PA05,DP
      break;
      default:
      break;
    }
  }

  switch(Pos)
  {
    case 1: GPIO_WritePin(CW_GPIOA,GPIO_PIN_8,GPIO_Pin_RESET); break;
    case 2: GPIO_WritePin(CW_GPIOA,GPIO_PIN_11,GPIO_Pin_RESET); break;
    case 3: GPIO_WritePin(CW_GPIOA,GPIO_PIN_12,GPIO_Pin_RESET); break;
    case 4: GPIO_WritePin(CW_GPIOA,GPIO_PIN_15,GPIO_Pin_RESET); break;
    case 5: GPIO_WritePin(CW_GPIOB,GPIO_PIN_3,GPIO_Pin_RESET); break;
    case 6: GPIO_WritePin(CW_GPIOB,GPIO_PIN_4,GPIO_Pin_RESET); break;
    default: break;
  }
}

static volatile uint8_t Seg_Reg[6] = {10,0,0,10,0,0};

/**
 * @brief 关闭所有公共端
 *
 */
void Close_Com(void)
{
  GPIO_WritePin(CW_GPIOA,GPIO_PIN_8,GPIO_Pin_SET);   //COM1:PA08
  GPIO_WritePin(CW_GPIOA,GPIO_PIN_11,GPIO_Pin_SET);  //COM2:PA11
  GPIO_WritePin(CW_GPIOA,GPIO_PIN_12,GPIO_Pin_SET);  //COM3:PA12
  GPIO_WritePin(CW_GPIOA,GPIO_PIN_15,GPIO_Pin_SET);  //COM4:PA15
  GPIO_WritePin(CW_GPIOB,GPIO_PIN_3,GPIO_Pin_SET);   //COM5:PB03
  GPIO_WritePin(CW_GPIOB,GPIO_PIN_4,GPIO_Pin_SET);   //COM6:PB04
}

/* value 单位为 0.01 V 或 0.01 A，三位显示自动选择 x.xx 或 xx.x。 */
static void Format_Value(uint32_t value, uint8_t digits[3])
{
  uint32_t tenths;
  if (value < 1000UL)
  {
    digits[0] = (uint8_t)(value / 100UL) + 10U;
    digits[1] = (uint8_t)(value / 10UL % 10UL);
    digits[2] = (uint8_t)(value % 10UL);
  }
  else
  {
    if (value > 9990UL) value = 9990UL;
    tenths = (value + 5UL) / 10UL;
    digits[0] = (uint8_t)(tenths / 100UL);
    digits[1] = (uint8_t)(tenths / 10UL % 10UL) + 10U;
    digits[2] = (uint8_t)(tenths % 10UL);
  }
}

static void Set_Display_Row(uint8_t offset, uint32_t value)
{
  uint8_t digits[3];
  uint32_t primask;
  uint8_t i;
  Format_Value(value, digits);
  primask = __get_PRIMASK();
  __disable_irq();
  for (i = 0; i < 3; i++) Seg_Reg[offset + i] = digits[i];
  __set_PRIMASK(primask);
}

void Display(uint32_t value)
{
  Set_Display_Row(0, value); /* 上排 COM1..COM3：电压 */
}

void DisplayI(uint32_t value)
{
  Set_Display_Row(3, value); /* 下排 COM4..COM6：电流 */
}

/* 保留原有电压显示接口。 */
void Dynamic_Display(uint32_t value)
{
  Display(value);
}

/* 每 1 ms 调用；每轮起始复制六位，整轮使用同一份段码。 */
void Display_Refresh(void)
{
  static uint8_t num = 0;
  static uint8_t frame[6];
  uint8_t i;
  if (num == 0)
  {
    for (i = 0; i < 6; i++) frame[i] = Seg_Reg[i];
  }
  Seg_Dis(num + 1U, frame[num]);
  num++;
  if (num >= 6) num = 0;
}

/* 一次更新六位，保证模式标签与读数在扫描端看到同一帧。
 * 正常：上排V、下排A。标定：上排S05/S15/A0.5/A1.5，下排为当前读数。
 * 七段管字母S与数字5外形相同；此处S表示电压标定。
 */
void Display_Mode(uint8_t mode, uint32_t voltage, uint32_t current, uint8_t error)
{
  uint8_t digits[6];
  uint8_t i;
  uint32_t primask;
  if (mode == 0)
  {
    Format_Value(voltage, digits);
    Format_Value(current, digits + 3);
  }
  else
  {
    if (mode <= 2)
    {
      digits[0] = 20; /* S */
      digits[1] = mode == 1 ? 0 : 1;
      digits[2] = 5;
      Format_Value(voltage, digits + 3);
    }
    else
    {
      digits[0] = 21; /* A */
      digits[1] = mode == 3 ? 10 : 11; /* 0.或1. */
      digits[2] = 5;
      Format_Value(current, digits + 3);
    }
  }
  if (error != 0)
  {
    digits[3] = 22; /* E01未采满，E02点位无效，E03保存失败。 */
    digits[4] = (uint8_t)(error / 10U % 10U);
    digits[5] = (uint8_t)(error % 10U);
  }
  primask = __get_PRIMASK();
  __disable_irq();
  for (i = 0; i < 6; i++) Seg_Reg[i] = digits[i];
  __set_PRIMASK(primask);
}

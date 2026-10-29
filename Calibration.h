#ifndef APP_CALIBRATION_H
#define APP_CALIBRATION_H

#include <stdint.h>

/* 64KB Flash 的末尾两页，每页 512 字节；工程链接区必须截止到 0xFC00。 */
#define CAL_FLASH_SLOT0 0x0000FC00UL
#define CAL_FLASH_SLOT1 0x0000FE00UL

/* Mode: 0正常；1电压5V；2电压15V；3电流0.5A；4电流1.5A。
 * 仅主循环修改模式和参数，中断只产生按键事件。
 * X05/X15/IX05/IX15 是实测 ADC 原始码值，不是电压或电流。
 */
extern uint8_t Mode;
extern uint16_t X05, X15, IX05, IX15;
extern float K, KI; /* 电压/电流斜率：每个 ADC 码对应多少个 0.01V/0.01A */

void read_vol_cur_calibration(void); /* 上电读取；无有效记录时使用理论系数 */
void ComputeK(void);                /* 由两点计算斜率和偏移 */
uint8_t save_calibration(void);     /* 主循环调用：1成功，0失败；禁止在中断中调用 */
uint32_t Calibration_Voltage(uint32_t code);
uint32_t Calibration_Current(uint32_t code);
uint8_t Calibration_Process(void);  /* 处理按键事件，返回是否需要更新显示 */
void DisplayBuff(void);             /* 按工作模式更新上下两排段码 */

#endif

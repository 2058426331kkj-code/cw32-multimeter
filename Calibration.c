#include "main.h"
#include "ADC.h"
#include "GPIO.h"
#include "Seg_Display.h"
#include "Calibration.h"

#define CAL_VERSION 1UL
#define CAL_MAGIC   0x43414C31UL
#define CAL_V_PAIR  0x03UL
#define CAL_I_PAIR  0x0CUL

/* 两页交替保存：先写新页，校验成功后才承认有效，不先擦除旧页。
 * magic 最后写入，掉电造成的不完整记录不会在下次上电时被采用。
 * 只保存整数 ADC 码，不保存浮点数，以免固件系数计算方式变化影响格式。
 */
typedef struct
{
  uint32_t version;
  uint32_t sequence;
  uint32_t mask;       /* 位0..3分别表示四个标定点是否已经实际采集 */
  uint32_t points[4];  /* 5V、15V、0.5A、1.5A 对应的滤波 ADC 码 */
  uint32_t checksum;
  uint32_t magic;
} CalibrationRecord;

uint8_t Mode = 0;
uint16_t X05 = 0, X15 = 0, IX05 = 0, IX15 = 0;
float K = 0.0f, KI = 0.0f;
static float V_Offset = 0.0f, I_Offset = 0.0f;
static uint32_t SavedMask = 0;
static uint32_t Sequence = 0;
static uint32_t ActiveSlot = 0; /* 0表示尚无有效Flash记录 */
static uint8_t ErrorCode = 0; /* 1未采满20点，2点位无效，3Flash保存失败 */

/* 对前7个32位字段做校验；magic 是提交标记，不参与校验。 */
static uint32_t RecordChecksum(const CalibrationRecord *r)
{
  uint32_t hash = 2166136261UL;
  uint32_t i;
  hash = (hash ^ r->version) * 16777619UL;
  hash = (hash ^ r->sequence) * 16777619UL;
  hash = (hash ^ r->mask) * 16777619UL;
  for (i = 0; i < 4; i++) hash = (hash ^ r->points[i]) * 16777619UL;
  return hash;
}

/* 无效/相同/反向的点不能用于除法。最小间隔仅防止误操作产生极大斜率，
 * 不代替万用表和标准电流源确认。仅采集一个点时暂时使用该路理论系数。
 */
static uint8_t PointsValid(const CalibrationRecord *r)
{
  uint32_t i;
  if (r->mask & ~0x0FUL) return 0;
  for (i = 0; i < 4; i++)
  {
    if (r->points[i] > 4095UL) return 0;
    if ((r->mask & (1UL << i)) && (r->points[i] == 0)) return 0;
  }
  if ((r->mask & CAL_V_PAIR) == CAL_V_PAIR &&
      r->points[1] < r->points[0] + 16UL) return 0;
  if ((r->mask & CAL_I_PAIR) == CAL_I_PAIR &&
      r->points[3] < r->points[2] + 8UL) return 0;
  return 1;
}

static uint8_t RecordValid(const CalibrationRecord *r)
{
  return r->magic == CAL_MAGIC && r->version == CAL_VERSION &&
         r->checksum == RecordChecksum(r) && PointsValid(r);
}

static void ReadRecord(uint32_t address, CalibrationRecord *r)
{
  /* 按字段读取，避免结构填充和未对齐访问；所有地址均为4字节对齐。 */
  const volatile uint32_t *p = (const volatile uint32_t *)address;
  uint32_t i;
  r->version = p[0]; r->sequence = p[1]; r->mask = p[2];
  for (i = 0; i < 4; i++) r->points[i] = p[3 + i];
  r->checksum = p[7]; r->magic = p[8];
}

static void CurrentRecord(CalibrationRecord *r)
{
  r->version = CAL_VERSION;
  r->sequence = Sequence + 1UL;
  r->mask = SavedMask;
  r->points[0] = X05; r->points[1] = X15;
  r->points[2] = IX05; r->points[3] = IX15;
  r->checksum = RecordChecksum(r);
  r->magic = CAL_MAGIC;
}

void ComputeK(void)
{
  /* 默认系数来自1.5V参考、220:10分压和100毫欧采样电阻。
   * y单位为0.01V/0.01A，x单位为ADC码：y = K*x + b。
   * 参考材料称三点，但五种模式只采两个非零点；这里由两点求直线，
   * 不宣称已经单独测得零点，负结果在输出函数中钳位为0。
   */
  K = (float)ADC_REF_VALUE * (R1 + R2) / (4096.0f * R1 * 10.0f);
  KI = (float)ADC_REF_VALUE * 100.0f / (4096.0f * CURRENT_SHUNT_MOHM);
  V_Offset = 0.0f; I_Offset = 0.0f;
  if ((SavedMask & CAL_V_PAIR) == CAL_V_PAIR && X15 >= X05 + 16U)
  {
    K = (1500.0f - 500.0f) / (float)(X15 - X05);
    V_Offset = 500.0f - K * X05;
  }
  if ((SavedMask & CAL_I_PAIR) == CAL_I_PAIR && IX15 >= IX05 + 8U)
  {
    KI = (150.0f - 50.0f) / (float)(IX15 - IX05);
    I_Offset = 50.0f - KI * IX05;
  }
}

void read_vol_cur_calibration(void)
{
  CalibrationRecord a, b;
  const CalibrationRecord *chosen = 0;
  uint8_t va, vb;
  SavedMask = 0; Sequence = 0; ActiveSlot = 0;
  X05 = X15 = IX05 = IX15 = 0;
  Mode = 0; ErrorCode = 0;
  ReadRecord(CAL_FLASH_SLOT0, &a);
  ReadRecord(CAL_FLASH_SLOT1, &b);
  va = RecordValid(&a); vb = RecordValid(&b);
  if (va) { chosen = &a; ActiveSlot = CAL_FLASH_SLOT0; }
  if (vb && (!va || (int32_t)(b.sequence - a.sequence) > 0))
  {
    chosen = &b; ActiveSlot = CAL_FLASH_SLOT1;
  }
  if (chosen != 0)
  {
    X05 = (uint16_t)chosen->points[0]; X15 = (uint16_t)chosen->points[1];
    IX05 = (uint16_t)chosen->points[2]; IX15 = (uint16_t)chosen->points[3];
    SavedMask = chosen->mask; Sequence = chosen->sequence;
  }
  ComputeK(); /* 首次上电/数据损坏也能使用理论换算，绝不除以0。 */
}

uint8_t save_calibration(void)
{
  CalibrationRecord next, verify;
  uint32_t address, primask;
  uint16_t locks;
  uint8_t page, status;
  CurrentRecord(&next);
  if (!PointsValid(&next)) return 0;
  address = ActiveSlot == CAL_FLASH_SLOT0 ? CAL_FLASH_SLOT1 : CAL_FLASH_SLOT0;
  page = (uint8_t)(address / 512UL);

  /* Flash擦写时先灭屏，避免扫描暂停导致某一位异常亮。
   * 暂停中断，防止Flash操作期间执行扫描；完成后恢复原中断和页面锁状态。
   * 驱动的 FLASH_WirteBytes 是原厂接口的拼写，不能改成 WriteBytes。
   */
  primask = __get_PRIMASK();
  __disable_irq();
  Close_Com();
  locks = (uint16_t)CW_FLASH->PAGELOCK;
  status = FLASH_UnlockPage(page);
  if (status == 0) status = FLASH_ErasePage(page);
  if (status == 0) status = FLASH_WirteBytes(address, (uint8_t *)&next, 32);
  if (status == 0) status = FLASH_WirteBytes(address + 32UL, (uint8_t *)&next.magic, 4);
  CW_FLASH->PAGELOCK = 0x5A5A0000UL | locks;
  __set_PRIMASK(primask);

  ReadRecord(address, &verify);
  if (status != 0 || !RecordValid(&verify) ||
      memcmp(&next, &verify, sizeof(next)) != 0) return 0;
  ActiveSlot = address; Sequence = next.sequence;
  return 1;
}

static uint32_t CalibratedValue(uint32_t code, float slope, float offset)
{
  float value = slope * (float)code + offset;
  if (value <= 0.0f) return 0;
  /* 三位显示最大99.9；先限幅，再转整数，避免错误参数导致转换溢出。 */
  if (value >= 9990.0f) return 9990UL;
  return (uint32_t)(value + 0.5f);
}

uint32_t Calibration_Voltage(uint32_t code)
{
  return CalibratedValue(code, K, V_Offset);
}
uint32_t Calibration_Current(uint32_t code)
{
  return CalibratedValue(code, KI, I_Offset);
}

static uint8_t CapturePoint(void)
{
  uint16_t volts[ADC_SAMPLE_SIZE], currents[ADC_SAMPLE_SIZE];
  uint16_t old[4];
  uint32_t oldMask = SavedMask;
  uint16_t code;
  CalibrationRecord candidate;
  old[0] = X05; old[1] = X15; old[2] = IX05; old[3] = IX15;
  if (!ADC_CopySamples(volts, currents)) return 1;
  code = (uint16_t)Mean_Value_Filter(Mode <= 2 ? volts : currents, ADC_SAMPLE_SIZE);
  switch (Mode)
  {
    case 1: X05 = code; break;
    case 2: X15 = code; break;
    case 3: IX05 = code; break;
    case 4: IX15 = code; break;
    default: return 2;
  }
  SavedMask |= 1UL << (Mode - 1U);
  CurrentRecord(&candidate);
  if (!PointsValid(&candidate)) ErrorCode = 2;
  else if (!save_calibration()) ErrorCode = 3;
  else { ComputeK(); return 0; }
  /* 失败时保持原参数、原模式，用户可调整输入并重新按K2。 */
  X05 = old[0]; X15 = old[1]; IX05 = old[2]; IX15 = old[3];
  SavedMask = oldMask;
  return ErrorCode;
}

uint8_t Calibration_Process(void)
{
  uint8_t events = KEY_GetEvents();
  if (events == 0) return 0;
  /* 同时按键时退出优先，其次切换模式，防止误保存。 */
  ErrorCode = 0;
  if (events & KEY_EVENT_EXIT) Mode = 0;
  else if (events & KEY_EVENT_MODE) Mode = (uint8_t)((Mode + 1U) % 5U);
  else if ((events & KEY_EVENT_SAVE) && Mode != 0)
  {
    ErrorCode = CapturePoint();
    if (ErrorCode == 0) Mode = 0;
  }
  Volt_Cal();
  return 1;
}

void DisplayBuff(void)
{
  /* 主循环更新整帧段码，中断只扫描；错误码保持到下一次按键。 */
  Display_Mode(Mode, V_Buffer, I_Buffer, ErrorCode);
}

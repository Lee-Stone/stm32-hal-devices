#ifndef __AHT20_H
#define __AHT20_H

#include "config.h"

#if DEVICE_AHT20

#include <stdint.h>

/* ==================== I2C 地址与指令 ==================== */

#define AHT20_ADDR              0x70  /* 7-bit: 0x38, 8-bit: 0x70 */
#define AHT20_CMD_INIT           0xBE  /* 初始化（校准）          */
#define AHT20_CMD_TRIGGER        0xAC  /* 触发测量                */
#define AHT20_CMD_RESET          0xBA  /* 软复位                  */

/* ==================== API 函数声明 ==================== */

void     AHT20_Init(void);
uint8_t  AHT20_ReadStatus(void);
void     AHT20_ReadData(float *temp, float *humi);

#endif /* DEVICE_AHT20 */

#endif /* __AHT20_H */

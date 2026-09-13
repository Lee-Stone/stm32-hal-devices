#ifndef __ICM42688P_H
#define __ICM42688P_H

#include "config.h"

#if DEVICE_ICM42688P

#include <stdint.h>

/* ==================== 芯片参数宏定义 ==================== */

#define ICM42688P_WHO_AM_I_VALUE       0x47U
#define ICM42688P_GYRO_SENSITIVITY     65.5f      /* ±500 dps，LSB/(°/s) */
#define ICM42688P_ACCEL_SENSITIVITY    8192.0f    /* ±4 g，LSB/g */
#define ICM42688P_TEMPERATURE_SCALE    132.48f
#define ICM42688P_TEMPERATURE_OFFSET   25.0f
#define ICM42688P_SPI_DELAY_CYCLES     12U
#define ICM42688P_OK                   0U
#define ICM42688P_ERROR                1U

/* ==================== 类型定义 ==================== */

typedef struct
{
    int16_t x;
    int16_t y;
    int16_t z;
} ICM42688P_RawVector_t;

typedef struct
{
    float x;
    float y;
    float z;
} ICM42688P_Vector_t;

typedef struct
{
    ICM42688P_RawVector_t accel;
    ICM42688P_RawVector_t gyro;
    int16_t temperature;
} ICM42688P_RawData_t;

typedef struct
{
    ICM42688P_Vector_t accel_g;
    ICM42688P_Vector_t gyro_dps;
    float temperature_c;
} ICM42688P_Data_t;

/* ==================== API 函数声明 ==================== */

uint8_t  ICM42688P_Init(void);                                      // 初始化 ICM-42688-P
uint8_t  ICM42688P_ReadID(void);                                    // 读取 WHO_AM_I，正常值为 0x47
uint8_t  ICM42688P_ReadRaw(ICM42688P_RawData_t *raw_data);          // 读取温度、加速度和角速度原始值
uint8_t  ICM42688P_ReadData(ICM42688P_Data_t *data);                // 读取加速度 g、角速度 dps 和温度 °C

#endif /* DEVICE_ICM42688P */

#endif /* __ICM42688P_H */

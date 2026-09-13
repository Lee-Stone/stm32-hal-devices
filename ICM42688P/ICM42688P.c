#include "ICM42688P.h"

#if DEVICE_ICM42688P

/* ==================== 寄存器与配置宏定义 ==================== */

#define ICM42688P_REG_DEVICE_CONFIG    0x11U
#define ICM42688P_REG_TEMP_DATA1       0x1DU
#define ICM42688P_REG_PWR_MGMT0        0x4EU
#define ICM42688P_REG_GYRO_CONFIG0     0x4FU
#define ICM42688P_REG_ACCEL_CONFIG0    0x50U
#define ICM42688P_REG_WHO_AM_I         0x75U
#define ICM42688P_REG_BANK_SEL         0x76U

#define ICM42688P_READ_FLAG            0x80U
#define ICM42688P_SOFT_RESET           0x01U
#define ICM42688P_BANK_0               0x00U
#define ICM42688P_GYRO_CONFIG_VALUE    0x48U  /* ±500 dps，100 Hz */
#define ICM42688P_ACCEL_CONFIG_VALUE   0x48U  /* ±4 g，100 Hz */
#define ICM42688P_POWER_CONFIG_VALUE   0x0FU  /* 加速度计和陀螺仪低噪声模式 */
#define ICM42688P_SAMPLE_LENGTH        14U

/* ==================== 软件 SPI 底层驱动 ==================== */

/**
  * @brief  软件 SPI 半周期延时
  * @param  无
  * @retval 无
  */
static void icm42688p_spi_delay(void)
{
    volatile uint32_t count;

    for (count = 0U; count < ICM42688P_SPI_DELAY_CYCLES; count++)
    {
        __NOP();
    }
}

/**
  * @brief  软件 SPI Mode 0 传输一个字节
  * @param  data 要发送的字节
  * @retval 同时接收到的字节
  */
static uint8_t icm42688p_spi_swap_byte(uint8_t data)
{
    uint8_t received = 0U;
    uint8_t bit;

    for (bit = 0U; bit < 8U; bit++)
    {
        ICM42688P_MOSI((data & 0x80U) != 0U);
        data <<= 1U;
        icm42688p_spi_delay();

        ICM42688P_SCK(1U);
        received <<= 1U;
        if (ICM42688P_MISO != GPIO_PIN_RESET)
        {
            received |= 0x01U;
        }
        icm42688p_spi_delay();
        ICM42688P_SCK(0U);
    }
    return received;
}

/**
  * @brief  写入一个 ICM-42688-P 寄存器
  * @param  reg 寄存器地址
  * @param  value 写入值
  * @retval 无
  */
static void icm42688p_write_reg(uint8_t reg, uint8_t value)
{
    ICM42688P_CS(0U);
    (void)icm42688p_spi_swap_byte(reg & (uint8_t)~ICM42688P_READ_FLAG);
    (void)icm42688p_spi_swap_byte(value);
    ICM42688P_CS(1U);
}

/**
  * @brief  读取一个 ICM-42688-P 寄存器
  * @param  reg 寄存器地址
  * @retval 寄存器值
  */
static uint8_t icm42688p_read_reg(uint8_t reg)
{
    uint8_t value;

    ICM42688P_CS(0U);
    (void)icm42688p_spi_swap_byte(reg | ICM42688P_READ_FLAG);
    value = icm42688p_spi_swap_byte(0x00U);
    ICM42688P_CS(1U);
    return value;
}

/**
  * @brief  连续读取 ICM-42688-P 寄存器
  * @param  reg 起始寄存器地址
  * @param  data 接收缓冲区
  * @param  length 读取字节数
  * @retval 无
  */
static void icm42688p_read_regs(uint8_t reg, uint8_t *data, uint8_t length)
{
    uint8_t index;

    ICM42688P_CS(0U);
    (void)icm42688p_spi_swap_byte(reg | ICM42688P_READ_FLAG);
    for (index = 0U; index < length; index++)
    {
        data[index] = icm42688p_spi_swap_byte(0x00U);
    }
    ICM42688P_CS(1U);
}

/* ==================== 数据处理函数 ==================== */

/**
  * @brief  将两个大端字节转换为有符号原始值
  * @param  high_byte 高字节
  * @param  low_byte 低字节
  * @retval 16 位有符号原始值
  */
static int16_t icm42688p_make_int16(uint8_t high_byte, uint8_t low_byte)
{
    return (int16_t)(((uint16_t)high_byte << 8U) | (uint16_t)low_byte);
}

/**
  * @brief  将原始数据换算为常用物理量
  * @param  raw_data 原始数据
  * @param  data 换算结果
  * @retval 无
  */
static void icm42688p_convert(const ICM42688P_RawData_t *raw_data,
                             ICM42688P_Data_t *data)
{
    data->accel_g.x = (float)raw_data->accel.x / ICM42688P_ACCEL_SENSITIVITY;
    data->accel_g.y = (float)raw_data->accel.y / ICM42688P_ACCEL_SENSITIVITY;
    data->accel_g.z = (float)raw_data->accel.z / ICM42688P_ACCEL_SENSITIVITY;

    data->gyro_dps.x = (float)raw_data->gyro.x / ICM42688P_GYRO_SENSITIVITY;
    data->gyro_dps.y = (float)raw_data->gyro.y / ICM42688P_GYRO_SENSITIVITY;
    data->gyro_dps.z = (float)raw_data->gyro.z / ICM42688P_GYRO_SENSITIVITY;

    data->temperature_c = (float)raw_data->temperature /
                          ICM42688P_TEMPERATURE_SCALE +
                          ICM42688P_TEMPERATURE_OFFSET;
}

/* ==================== API 函数实现 ==================== */

/**
  * @brief  初始化 ICM-42688-P 六轴传感器
  * @param  无
  * @retval ICM42688P_OK 初始化成功，ICM42688P_ERROR 初始化失败
  */
uint8_t ICM42688P_Init(void)
{
    ICM42688P_CS(1U);
    ICM42688P_SCK(0U);
    ICM42688P_MOSI(0U);
    HAL_Delay(100U);

    icm42688p_write_reg(ICM42688P_REG_BANK_SEL, ICM42688P_BANK_0);
    icm42688p_write_reg(ICM42688P_REG_DEVICE_CONFIG, ICM42688P_SOFT_RESET);
    HAL_Delay(10U);

    icm42688p_write_reg(ICM42688P_REG_BANK_SEL, ICM42688P_BANK_0);
    if (ICM42688P_ReadID() != ICM42688P_WHO_AM_I_VALUE)
    {
        return ICM42688P_ERROR;
    }

    icm42688p_write_reg(ICM42688P_REG_GYRO_CONFIG0,
                        ICM42688P_GYRO_CONFIG_VALUE);
    icm42688p_write_reg(ICM42688P_REG_ACCEL_CONFIG0,
                        ICM42688P_ACCEL_CONFIG_VALUE);
    icm42688p_write_reg(ICM42688P_REG_PWR_MGMT0,
                        ICM42688P_POWER_CONFIG_VALUE);
    HAL_Delay(50U);

    return ICM42688P_OK;
}

/**
  * @brief  读取 ICM-42688-P 芯片标识
  * @param  无
  * @retval WHO_AM_I 寄存器值，正常为 0x47
  */
uint8_t ICM42688P_ReadID(void)
{
    return icm42688p_read_reg(ICM42688P_REG_WHO_AM_I);
}

/**
  * @brief  读取温度、三轴加速度和三轴角速度原始值
  * @param  raw_data 原始数据输出地址
  * @retval ICM42688P_OK 读取成功，ICM42688P_ERROR 参数或状态错误
  */
uint8_t ICM42688P_ReadRaw(ICM42688P_RawData_t *raw_data)
{
    uint8_t sample[ICM42688P_SAMPLE_LENGTH];

    if (raw_data == NULL)
    {
        return ICM42688P_ERROR;
    }

    icm42688p_read_regs(ICM42688P_REG_TEMP_DATA1, sample,
                        ICM42688P_SAMPLE_LENGTH);
    raw_data->temperature = icm42688p_make_int16(sample[0], sample[1]);
    raw_data->accel.x = icm42688p_make_int16(sample[2], sample[3]);
    raw_data->accel.y = icm42688p_make_int16(sample[4], sample[5]);
    raw_data->accel.z = icm42688p_make_int16(sample[6], sample[7]);
    raw_data->gyro.x = icm42688p_make_int16(sample[8], sample[9]);
    raw_data->gyro.y = icm42688p_make_int16(sample[10], sample[11]);
    raw_data->gyro.z = icm42688p_make_int16(sample[12], sample[13]);
    return ICM42688P_OK;
}

/**
  * @brief  读取加速度、角速度和温度物理量
  * @param  data 换算数据输出地址
  * @retval ICM42688P_OK 读取成功，ICM42688P_ERROR 参数或状态错误
  */
uint8_t ICM42688P_ReadData(ICM42688P_Data_t *data)
{
    ICM42688P_RawData_t raw_data;

    if (data == NULL)
    {
        return ICM42688P_ERROR;
    }
    if (ICM42688P_ReadRaw(&raw_data) != ICM42688P_OK)
    {
        return ICM42688P_ERROR;
    }
    icm42688p_convert(&raw_data, data);
    return ICM42688P_OK;
}

#endif /* DEVICE_ICM42688P */

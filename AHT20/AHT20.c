#include "AHT20.h"

#if DEVICE_AHT20

/* ==================== 软件 I2C 底层驱动 ====================
   注意：SDA 引脚必须配置为 GPIO_Output_OpenDrain（不是 Push-Pull）。
   Open-Drain 模式下写 1 = 释放总线（由外部上拉电阻拉高）、写 0 = 拉低。
   读 SDA 时只需先写 1 释放总线即可直接读取，无需切换引脚方向。
   i2c_delay 根据 SystemCoreClock 动态计算，确保 SCL ≤ 400kHz。 */

/**
  * @brief  软件 I2C 延时（基于主频自适应）
  * @param  无
  * @retval 无
  */
static void aht20_i2c_delay(void)
{
    volatile uint32_t i;
    for (i = 0; i < (SystemCoreClock / 2000000); i++) { }
}

/**
  * @brief  I2C 起始信号
  * @param  无
  * @retval 无
  */
static void aht20_i2c_start(void)
{
    AHT20_W_SDA(1);
    aht20_i2c_delay();
    AHT20_W_SCL(1);
    aht20_i2c_delay();
    AHT20_W_SDA(0);
    aht20_i2c_delay();
    AHT20_W_SCL(0);
    aht20_i2c_delay();
}

/**
  * @brief  I2C 停止信号
  * @param  无
  * @retval 无
  */
static void aht20_i2c_stop(void)
{
    AHT20_W_SDA(0);
    aht20_i2c_delay();
    AHT20_W_SCL(1);
    aht20_i2c_delay();
    AHT20_W_SDA(1);
    aht20_i2c_delay();
}

/**
  * @brief  I2C 发送一个字节
  * @param  byte 要发送的字节
  * @retval 无
  */
static void aht20_i2c_sendbyte(uint8_t byte)
{
    uint8_t i;

    for (i = 0; i < 8; i++)
    {
        AHT20_W_SDA(byte & (0x80 >> i));
        aht20_i2c_delay();
        AHT20_W_SCL(1);
        aht20_i2c_delay();
        AHT20_W_SCL(0);
        aht20_i2c_delay();
    }
}

/**
  * @brief  I2C 接收一个字节
  * @param  ack 0 = 发送 NACK（最后字节）, 1 = 发送 ACK
  * @retval 接收到的字节
  */
static uint8_t aht20_i2c_recvbyte(uint8_t ack)
{
    uint8_t i, byte = 0;

    AHT20_W_SDA(1);     /* 释放 SDA 总线，准备读取 */
    aht20_i2c_delay();
    for (i = 0; i < 8; i++)
    {
        AHT20_W_SCL(1);
        aht20_i2c_delay();
        if (AHT20_R_SDA)
        {
            byte |= (0x80 >> i);
        }
        AHT20_W_SCL(0);
        aht20_i2c_delay();
    }

    /* 发送 ACK / NACK */
    if (ack)
    {
        AHT20_W_SDA(0);     /* ACK  */
    }
    else
    {
        AHT20_W_SDA(1);     /* NACK */
    }
    aht20_i2c_delay();
    AHT20_W_SCL(1);
    aht20_i2c_delay();
    AHT20_W_SCL(0);
    aht20_i2c_delay();

    return byte;
}

/**
  * @brief  I2C 等待从机应答
  * @param  无
  * @retval 0 = 收到 ACK, 1 = 未收到 ACK
  */
static uint8_t aht20_i2c_waitack(void)
{
    uint8_t ack;

    AHT20_W_SDA(1);     /* 释放 SDA 总线 */
    aht20_i2c_delay();
    AHT20_W_SCL(1);
    aht20_i2c_delay();
    ack = AHT20_R_SDA;  /* 0 = ACK, 1 = NACK */
    AHT20_W_SCL(0);
    aht20_i2c_delay();

    return ack;
}

/* ==================== AHT20 指令操作 ==================== */

/**
  * @brief  AHT20 软复位
  * @param  无
  * @retval 无
  */
static void aht20_reset(void)
{
    aht20_i2c_start();
    aht20_i2c_sendbyte(AHT20_ADDR | 0x00);
    aht20_i2c_waitack();
    aht20_i2c_sendbyte(AHT20_CMD_RESET);
    aht20_i2c_waitack();
    aht20_i2c_stop();
}

/**
  * @brief  AHT20 初始化（校准）
  * @param  无
  * @retval 无
  */
void AHT20_Init(void)
{
    HAL_Delay(40);      /* 上电等待（传感器就绪 ≥ 40ms） */

    aht20_reset();
    HAL_Delay(20);

    aht20_i2c_start();
    aht20_i2c_sendbyte(AHT20_ADDR | 0x00);
    aht20_i2c_waitack();
    aht20_i2c_sendbyte(AHT20_CMD_INIT);
    aht20_i2c_waitack();
    aht20_i2c_sendbyte(0x08);
    aht20_i2c_waitack();
    aht20_i2c_sendbyte(0x00);
    aht20_i2c_waitack();
    aht20_i2c_stop();

    HAL_Delay(10);  /* 等待校准完成（tCAL ≤ 10ms） */
}

/**
  * @brief  读取状态字节
  * @param  无
  * @retval 状态字节（bit[3]=1 已校准, bit[7]=1 忙）
  */
uint8_t AHT20_ReadStatus(void)
{
    uint8_t status = 0;

    aht20_i2c_start();
    aht20_i2c_sendbyte(AHT20_ADDR | 0x01);
    aht20_i2c_waitack();
    status = aht20_i2c_recvbyte(0);
    aht20_i2c_stop();

    return status;
}

/**
  * @brief  触发测量
  * @param  无
  * @retval 无
  */
static void aht20_trigger(void)
{
    aht20_i2c_start();
    aht20_i2c_sendbyte(AHT20_ADDR | 0x00);
    aht20_i2c_waitack();
    aht20_i2c_sendbyte(AHT20_CMD_TRIGGER);
    aht20_i2c_waitack();
    aht20_i2c_sendbyte(0x33);
    aht20_i2c_waitack();
    aht20_i2c_sendbyte(0x00);
    aht20_i2c_waitack();
    aht20_i2c_stop();
}

/**
  * @brief  读取温湿度数据
  * @param  temp 温度指针（°C）
  * @param  humi 湿度指针（%RH）
  * @retval 无
  */
void AHT20_ReadData(float *temp, float *humi)
{
    uint8_t buf[6];
    uint32_t raw_humi, raw_temp;
    uint8_t i;

    /* 触发测量并等待完成（tMEAS ≤ 75ms） */
    aht20_trigger();
    HAL_Delay(75);

    /* 读取 6 字节测量数据 */
    aht20_i2c_start();
    aht20_i2c_sendbyte(AHT20_ADDR | 0x01);
    aht20_i2c_waitack();
    for (i = 0; i < 6; i++)
    {
        buf[i] = aht20_i2c_recvbyte(i < 5 ? 1 : 0);
    }
    aht20_i2c_stop();

    /* 解析湿度（高 20 位） */
    raw_humi = ((uint32_t)buf[1] << 12)
             | ((uint32_t)buf[2] << 4)
             | ((uint32_t)buf[3] >> 4);

    /* 解析温度（低 20 位） */
    raw_temp = ((uint32_t)(buf[3] & 0x0F) << 16)
             | ((uint32_t)buf[4] << 8)
             |  (uint32_t)buf[5];

    /* 公式：RH% = raw / 2^20 * 100%,  T°C = raw / 2^20 * 200 - 50 */
    if (humi)
    {
        *humi = (float)raw_humi / 1048576.0f * 100.0f;
    }
    if (temp)
    {
        *temp = (float)raw_temp / 1048576.0f * 200.0f - 50.0f;
    }
}

#endif /* DEVICE_AHT20 */

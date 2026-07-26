#include "XPT2046.h"

#if DEVICE_XPT2046

/* ==================== 指令定义 ==================== */

#define XPT2046_CMD_X           0x90  /* 读 X 坐标（12 位，差分） */
#define XPT2046_CMD_Y           0xD0  /* 读 Y 坐标（12 位，差分） */

/* ==================== 软件 SPI 底层 ==================== */

/**
  * @brief  SPI 写一个字节并读回一个字节
  * @param  tx 发送的字节
  * @retval 读回的字节
  */
static uint8_t xpt2046_spi_rw(uint8_t tx)
{
    uint8_t rx = 0;
    uint8_t i;

    for (i = 0; i < 8; i++)
    {
        XPT2046_DIN(tx & 0x80);
        tx <<= 1;

        XPT2046_CLK(1);
        for (volatile uint32_t d = 0; d < (SystemCoreClock / 2000000); d++) { }
        XPT2046_CLK(0);

        rx <<= 1;
        if (XPT2046_DO)
        {
            rx |= 0x01;
        }
    }

    return rx;
}

/**
  * @brief  读取 ADC 原始值（12 位）
  * @param  cmd 通道选择命令
  * @retval 12 位 ADC 值
  */
static uint16_t xpt2046_read_adc(uint8_t cmd)
{
    uint16_t val;

    XPT2046_CS(0);

    xpt2046_spi_rw(cmd);                        /* 发送命令，丢弃首个返回字节 */
    val  = (uint16_t)xpt2046_spi_rw(0x00) << 4; /* D11 ~ D4               */
    val |= (uint16_t)xpt2046_spi_rw(0x00) >> 4; /* D3 ~ D0（高4位有效）     */

    XPT2046_CS(1);

    return val;     /* 12 位 ADC 结果 */
}

/* ==================== API 实现 ==================== */

/**
  * @brief  XPT2046 初始化
  * @param  无
  * @retval 无
  */
void XPT2046_Init(void)
{
    XPT2046_CS(1);
    XPT2046_CLK(0);
}

/**
  * @brief  判断是否有触摸
  * @param  无
  * @retval 1 = 按下, 0 = 未按下
  */
uint8_t XPT2046_IsPressed(void)
{
    return (XPT2046_IRQ == 0) ? 1 : 0;
}

/**
  * @brief  读取触摸坐标（已映射到屏幕分辨率）
  * @param  x  X 坐标指针（未按下时保持原值）
  * @param  y  Y 坐标指针（未按下时保持原值）
  * @retval 1 = 有触摸, 0 = 未按下
  */
uint8_t XPT2046_ReadXY(uint16_t *x, uint16_t *y)
{
    uint16_t raw_x, raw_y;
    int32_t  tmp;

    if (!XPT2046_IsPressed())
        return 0;

    /* 多次采样取平均（去抖） */
    raw_x = xpt2046_read_adc(XPT2046_CMD_X);
    raw_x += xpt2046_read_adc(XPT2046_CMD_X);
    raw_x >>= 1;

    raw_y = xpt2046_read_adc(XPT2046_CMD_Y);
    raw_y += xpt2046_read_adc(XPT2046_CMD_Y);
    raw_y >>= 1;

    /* 映射到屏幕坐标（横屏 320×240） */
    tmp = ((int32_t)(raw_x - XPT2046_X_MIN) * 319)
        / (XPT2046_X_MAX - XPT2046_X_MIN);
    if (tmp < 0)   tmp = 0;
    if (tmp > 319) tmp = 319;
    *x = (uint16_t)tmp;

    tmp = ((int32_t)(raw_y - XPT2046_Y_MIN) * 239)
        / (XPT2046_Y_MAX - XPT2046_Y_MIN);
    if (tmp < 0)   tmp = 0;
    if (tmp > 239) tmp = 239;
    *y = (uint16_t)tmp;

    return 1;
}

#endif /* DEVICE_XPT2046 */

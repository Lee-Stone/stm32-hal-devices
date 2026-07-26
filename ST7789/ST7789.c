#include "ST7789.h"

#if DEVICE_ST7789

#include "ST7789_Font.h"
#include <string.h>

/* ==================== 屏幕参数（运行时可变） ==================== */

uint16_t st7789_width  = 240;
uint16_t st7789_height = 320;

/* ==================== DMA 状态 ==================== */

static volatile uint8_t st7789_dma_busy = 0;

/**
  * @brief  SPI TX DMA 完成回调
  * @param  hspi SPI 句柄
  * @retval 无
  */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == ST7789_SPI.Instance)
    {
        st7789_dma_busy = 0;
    }
}

/**
  * @brief  等待 DMA 传输完成
  * @param  无
  * @retval 无
  */
static void st7789_wait_dma(void)
{
    while (st7789_dma_busy)
    {
        /* 等待 DMA 传输完成 */
    }
}

/* ==================== SPI 底层收发 ==================== */

/**
  * @brief  写命令（DC = 0，DMA）
  * @param  cmd 命令字节
  * @retval 无
  */
static void st7789_write_cmd(uint8_t cmd)
{
    ST7789_DC(0);
    ST7789_CS(0);
    st7789_dma_busy = 1;
    HAL_SPI_Transmit_DMA(&ST7789_SPI, &cmd, 1);
    st7789_wait_dma();
    ST7789_CS(1);
}

/**
  * @brief  写单字节数据（DC = 1，DMA）
  * @param  data 数据字节
  * @retval 无
  */
static void st7789_write_data8(uint8_t data)
{
    ST7789_DC(1);
    ST7789_CS(0);
    st7789_dma_busy = 1;
    HAL_SPI_Transmit_DMA(&ST7789_SPI, &data, 1);
    st7789_wait_dma();
    ST7789_CS(1);
}

/**
  * @brief  写双字节数据（DC = 1，RGB565 像素值，DMA）
  * @param  data 16 位数据
  * @retval 无
  */
static void st7789_write_data16(uint16_t data)
{
    uint8_t buf[2];
    buf[0] = (uint8_t)(data >> 8);
    buf[1] = (uint8_t)(data & 0xFF);

    ST7789_DC(1);
    ST7789_CS(0);
    st7789_dma_busy = 1;
    HAL_SPI_Transmit_DMA(&ST7789_SPI, buf, 2);
    st7789_wait_dma();
    ST7789_CS(1);
}

/**
  * @brief  批量写数据（DC = 1，通过 DMA 传输）
  * @param  data 数据缓冲区指针
  * @param  len  数据长度（字节）
  * @retval 无
  */
static void st7789_write_buf(const uint8_t *data, uint32_t len)
{
    ST7789_DC(1);
    ST7789_CS(0);
    st7789_dma_busy = 1;
    HAL_SPI_Transmit_DMA(&ST7789_SPI, (uint8_t *)data, (uint16_t)len);
    st7789_wait_dma();
    ST7789_CS(1);
}

/* ==================== 初始化 ==================== */

/**
  * @brief  ST7789 初始化（硬件复位 + 寄存器配置）
  * @param  无
  * @retval 无
  */
void ST7789_Init(void)
{
    /* 硬件复位 */
    ST7789_RST(0);
    HAL_Delay(10);
    ST7789_RST(1);
    HAL_Delay(120);

    /* 退出睡眠模式 */
    st7789_write_cmd(0x11);
    HAL_Delay(120);

    /* 设置像素格式：RGB565（16-bit） */
    st7789_write_cmd(0x3A);
    st7789_write_data8(0x55);

    /* 帧率控制 */
    st7789_write_cmd(0xB2);
    st7789_write_data8(0x0C);
    st7789_write_data8(0x0C);
    st7789_write_data8(0x00);
    st7789_write_data8(0x33);
    st7789_write_data8(0x33);

    /* 显示反转关 */
    st7789_write_cmd(0x20);

    /* VCOM 设置 */
    st7789_write_cmd(0xBB);
    st7789_write_data8(0x3B);

    /* 电源控制 */
    st7789_write_cmd(0xC2);
    st7789_write_data8(0x01);

    st7789_write_cmd(0xC3);
    st7789_write_data8(0x1A);

    st7789_write_cmd(0xC4);
    st7789_write_data8(0x20);

    st7789_write_cmd(0xC6);
    st7789_write_data8(0x0F);

    st7789_write_cmd(0xD0);
    st7789_write_data8(0xA4);
    st7789_write_data8(0xA1);

    /* Gamma 校正 */
    st7789_write_cmd(0xE0);
    st7789_write_data8(0xD0);
    st7789_write_data8(0x06);
    st7789_write_data8(0x0C);
    st7789_write_data8(0x09);
    st7789_write_data8(0x31);
    st7789_write_data8(0x35);
    st7789_write_data8(0x2F);
    st7789_write_data8(0x31);
    st7789_write_data8(0x37);
    st7789_write_data8(0x3A);
    st7789_write_data8(0x1A);
    st7789_write_data8(0x11);
    st7789_write_data8(0x14);
    st7789_write_data8(0x05);

    st7789_write_cmd(0xE1);
    st7789_write_data8(0xD0);
    st7789_write_data8(0x06);
    st7789_write_data8(0x0C);
    st7789_write_data8(0x09);
    st7789_write_data8(0x31);
    st7789_write_data8(0x35);
    st7789_write_data8(0x2F);
    st7789_write_data8(0x31);
    st7789_write_data8(0x37);
    st7789_write_data8(0x3A);
    st7789_write_data8(0x1A);
    st7789_write_data8(0x11);
    st7789_write_data8(0x14);
    st7789_write_data8(0x05);

    /* 正常显示模式 */
    st7789_write_cmd(0x13);

    /* 显示开 */
    st7789_write_cmd(0x29);

    /* 背光开 */
    ST7789_BLED(1);

    /* 默认横屏 */
    ST7789_SetRotation(3);
}

/**
  * @brief  设置屏幕旋转方向
  * @param  rotation 旋转模式：0=竖屏 1=横屏 2=竖屏翻转 3=横屏翻转
  * @retval 无
  */
void ST7789_SetRotation(uint8_t rotation)
{
    static const uint8_t madctl_table[] = { 0x00, 0x60, 0xC0, 0xA0 };

    st7789_write_cmd(0x36);
    st7789_write_data8(madctl_table[rotation & 0x03]);

    if (rotation & 0x01)
    {
        st7789_width  = 320;
        st7789_height = 240;
    }
    else
    {
        st7789_width  = 240;
        st7789_height = 320;
    }
}

/* ==================== 画图 API ==================== */

/**
  * @brief  设置像素写入窗口
  * @param  x  起始 X 坐标
  * @param  y  起始 Y 坐标
  * @param  w  窗口宽度
  * @param  h  窗口高度
  * @retval 无
  */
void ST7789_SetWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    uint16_t xe = x + w - 1;
    uint16_t ye = y + h - 1;

    st7789_write_cmd(0x2A);     /* CASET 列地址 */
    st7789_write_data16(x);
    st7789_write_data16(xe);

    st7789_write_cmd(0x2B);     /* RASET 行地址 */
    st7789_write_data16(y);
    st7789_write_data16(ye);

    st7789_write_cmd(0x2C);     /* RAMWR 写内存 */
}

/**
  * @brief  批量写入像素数据（DMA 方式，配合 ST7789_SetWindow 使用）
  * @param  data 像素数据缓冲区
  * @param  len  数据长度（字节）
  * @retval 无
  */
void ST7789_WriteData(const uint8_t *data, uint32_t len)
{
    st7789_write_buf(data, len);
}

/**
  * @brief  全屏填充
  * @param  color 填充颜色（RGB565）
  * @retval 无
  */
void ST7789_Clear(uint16_t color)
{
    uint8_t  fill[1024];
    uint8_t  hi = (uint8_t)(color >> 8);
    uint8_t  lo = (uint8_t)(color);
    uint16_t xe = ST7789_WIDTH - 1;
    uint16_t ye = ST7789_HEIGHT - 1;
    uint32_t i, sent = 0;
    uint32_t total = (uint32_t)ST7789_WIDTH * ST7789_HEIGHT * 2;

    /* 预填颜色缓冲 */
    for (i = 0; i < sizeof(fill); i += 2)
    {
        fill[i]     = hi;
        fill[i + 1] = lo;
    }

    /* ====== 一次 CS 周期内发完 窗口设置 + 全部像素数据 ====== */

    ST7789_CS(0);

    /* CASET: 列地址 */
    ST7789_DC(0);
    uint8_t caset = 0x2A;
    st7789_dma_busy = 1;
    HAL_SPI_Transmit_DMA(&ST7789_SPI, &caset, 1);
    st7789_wait_dma();
    ST7789_DC(1);
    uint8_t caset_buf[4] = { 0, 0, (uint8_t)(xe >> 8), (uint8_t)(xe) };
    st7789_dma_busy = 1;
    HAL_SPI_Transmit_DMA(&ST7789_SPI, caset_buf, 4);
    st7789_wait_dma();

    /* RASET: 行地址 */
    ST7789_DC(0);
    uint8_t raset = 0x2B;
    st7789_dma_busy = 1;
    HAL_SPI_Transmit_DMA(&ST7789_SPI, &raset, 1);
    st7789_wait_dma();
    ST7789_DC(1);
    uint8_t raset_buf[4] = { 0, 0, (uint8_t)(ye >> 8), (uint8_t)(ye) };
    st7789_dma_busy = 1;
    HAL_SPI_Transmit_DMA(&ST7789_SPI, raset_buf, 4);
    st7789_wait_dma();

    /* RAMWR: 写内存 + 像素数据 */
    ST7789_DC(0);
    uint8_t ramwr = 0x2C;
    st7789_dma_busy = 1;
    HAL_SPI_Transmit_DMA(&ST7789_SPI, &ramwr, 1);
    st7789_wait_dma();
    ST7789_DC(1);
    while (sent < total)
    {
        uint32_t chunk = total - sent;
        if (chunk > sizeof(fill)) chunk = sizeof(fill);
        st7789_dma_busy = 1;
        HAL_SPI_Transmit_DMA(&ST7789_SPI, fill, (uint16_t)chunk);
        st7789_wait_dma();
        sent += chunk;
    }

    ST7789_CS(1);
}

/**
  * @brief  画单个像素点
  * @param  x     X 坐标
  * @param  y     Y 坐标
  * @param  color 颜色（RGB565）
  * @retval 无
  */
void ST7789_DrawPoint(uint16_t x, uint16_t y, uint16_t color)
{
    uint8_t hi = (uint8_t)(color >> 8);
    uint8_t lo = (uint8_t)(color);

    ST7789_CS(0);

    /* CASET */
    ST7789_DC(0);
    uint8_t caset = 0x2A;
    st7789_dma_busy = 1;
    HAL_SPI_Transmit_DMA(&ST7789_SPI, &caset, 1);
    st7789_wait_dma();
    ST7789_DC(1);
    uint8_t caset_buf[4] = { (uint8_t)(x >> 8), (uint8_t)(x),
                             (uint8_t)(x >> 8), (uint8_t)(x) };
    st7789_dma_busy = 1;
    HAL_SPI_Transmit_DMA(&ST7789_SPI, caset_buf, 4);
    st7789_wait_dma();

    /* RASET */
    ST7789_DC(0);
    uint8_t raset = 0x2B;
    st7789_dma_busy = 1;
    HAL_SPI_Transmit_DMA(&ST7789_SPI, &raset, 1);
    st7789_wait_dma();
    ST7789_DC(1);
    uint8_t raset_buf[4] = { (uint8_t)(y >> 8), (uint8_t)(y),
                             (uint8_t)(y >> 8), (uint8_t)(y) };
    st7789_dma_busy = 1;
    HAL_SPI_Transmit_DMA(&ST7789_SPI, raset_buf, 4);
    st7789_wait_dma();

    /* RAMWR + 像素 */
    ST7789_DC(0);
    uint8_t ramwr = 0x2C;
    st7789_dma_busy = 1;
    HAL_SPI_Transmit_DMA(&ST7789_SPI, &ramwr, 1);
    st7789_wait_dma();
    ST7789_DC(1);
    uint8_t pixel[2] = { hi, lo };
    st7789_dma_busy = 1;
    HAL_SPI_Transmit_DMA(&ST7789_SPI, pixel, 2);
    st7789_wait_dma();

    ST7789_CS(1);
}

/**
  * @brief  填充矩形区域
  * @param  x     X 坐标
  * @param  y     Y 坐标
  * @param  w     宽度
  * @param  h     高度
  * @param  color 颜色（RGB565）
  * @retval 无
  */
void ST7789_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    uint8_t  fill[1024];
    uint8_t  hi = (uint8_t)(color >> 8);
    uint8_t  lo = (uint8_t)(color);
    uint16_t xe = x + w - 1;
    uint16_t ye = y + h - 1;
    uint32_t i, sent = 0;
    uint32_t total = (uint32_t)w * h * 2;

    for (i = 0; i < sizeof(fill); i += 2)
    {
        fill[i]     = hi;
        fill[i + 1] = lo;
    }

    ST7789_CS(0);

    /* CASET */
    ST7789_DC(0);
    uint8_t caset = 0x2A;
    st7789_dma_busy = 1;
    HAL_SPI_Transmit_DMA(&ST7789_SPI, &caset, 1);
    st7789_wait_dma();
    ST7789_DC(1);
    uint8_t caset_buf[4] = { (uint8_t)(x >> 8), (uint8_t)(x),
                             (uint8_t)(xe >> 8), (uint8_t)(xe) };
    st7789_dma_busy = 1;
    HAL_SPI_Transmit_DMA(&ST7789_SPI, caset_buf, 4);
    st7789_wait_dma();

    /* RASET */
    ST7789_DC(0);
    uint8_t raset = 0x2B;
    st7789_dma_busy = 1;
    HAL_SPI_Transmit_DMA(&ST7789_SPI, &raset, 1);
    st7789_wait_dma();
    ST7789_DC(1);
    uint8_t raset_buf[4] = { (uint8_t)(y >> 8), (uint8_t)(y),
                             (uint8_t)(ye >> 8), (uint8_t)(ye) };
    st7789_dma_busy = 1;
    HAL_SPI_Transmit_DMA(&ST7789_SPI, raset_buf, 4);
    st7789_wait_dma();

    /* RAMWR + 像素数据 */
    ST7789_DC(0);
    uint8_t ramwr = 0x2C;
    st7789_dma_busy = 1;
    HAL_SPI_Transmit_DMA(&ST7789_SPI, &ramwr, 1);
    st7789_wait_dma();
    ST7789_DC(1);
    while (sent < total)
    {
        uint32_t chunk = total - sent;
        if (chunk > sizeof(fill)) chunk = sizeof(fill);
        st7789_dma_busy = 1;
        HAL_SPI_Transmit_DMA(&ST7789_SPI, fill, (uint16_t)chunk);
        st7789_wait_dma();
        sent += chunk;
    }

    ST7789_CS(1);
}

/**
  * @brief  显示图片（16 位色 RGB565）
  * @param  x    X 坐标
  * @param  y    Y 坐标
  * @param  w    宽度
  * @param  h    高度
  * @param  data 图片像素数据指针
  * @retval 无
  */
void ST7789_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                      const uint16_t *data)
{
    ST7789_SetWindow(x, y, w, h);
    st7789_write_buf((const uint8_t *)data, (uint32_t)w * h * 2);
}

/* ==================== 字符显示 ==================== */

/**
  * @brief  在指定位置写一个字符
  * @param  x     X 坐标
  * @param  y     Y 坐标
  * @param  ch    字符
  * @param  color 颜色（RGB565）
  * @param  size  字体大小（12 或 16）
  * @retval 无
  *
  * 字模数据格式：每个 byte 代表一列 8 个纵向像素（bit 0 = 顶部）。
  * 16 号字体：byte[0..7] 为上半（行 0~7），byte[8..15] 为下半（行 8~15）。
  */
static void st7789_show_char(uint16_t x, uint16_t y, char ch, uint16_t color)
{
    uint8_t chr = ch - ' ';
    uint8_t col, bit;

    if (chr > 94) return;

    for (col = 0; col < 8; col++)
    {
        uint8_t upper = ST7789_Font8x16[chr][col];
        uint8_t lower = ST7789_Font8x16[chr][col + 8];

        for (bit = 0; bit < 8; bit++)
        {
            if (upper & 0x01)
                ST7789_DrawPoint(x + col, y + bit, color);
            upper >>= 1;
        }
        for (bit = 0; bit < 8; bit++)
        {
            if (lower & 0x01)
                ST7789_DrawPoint(x + col, y + 8 + bit, color);
            lower >>= 1;
        }
    }
}

/**
  * @brief  在指定位置写字符串（默认 8×16 字体）
  * @param  x     X 坐标
  * @param  y     Y 坐标
  * @param  str   字符串（以 '\0' 结尾）
  * @param  color 颜色（RGB565）
  * @retval 无
  */
void ST7789_WriteString(uint16_t x, uint16_t y, char *str, uint16_t color)
{
    while (*str)
    {
        if (x + 8 > ST7789_WIDTH)
        {
            x = 0;
            y += 16;
        }
        if (y + 16 > ST7789_HEIGHT)
        {
            y = 0;
        }

        st7789_show_char(x, y, *str, color);
        x += 8;
        str++;
    }
}

#endif /* DEVICE_ST7789 */

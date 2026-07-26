#ifndef __ST7789_H
#define __ST7789_H

#include "config.h"

#if DEVICE_ST7789

#include <stdint.h>

/* ==================== 屏幕参数（运行时可变，配合旋转） ==================== */

extern uint16_t st7789_width;
extern uint16_t st7789_height;
#define ST7789_WIDTH            (st7789_width)
#define ST7789_HEIGHT           (st7789_height)

/* ==================== 颜色定义（RGB565） ==================== */

#define ST7789_COLOR_BLACK      0x0000
#define ST7789_COLOR_WHITE      0xFFFF
#define ST7789_COLOR_RED        0xF800
#define ST7789_COLOR_GREEN      0x07E0
#define ST7789_COLOR_BLUE       0x001F
#define ST7789_COLOR_YELLOW     0xFFE0
#define ST7789_COLOR_CYAN       0x07FF
#define ST7789_COLOR_MAGENTA    0xF81F

/* ==================== API 函数声明 ==================== */

void     ST7789_Init(void);
void     ST7789_SetRotation(uint8_t rotation);
void     ST7789_Clear(uint16_t color);
void     ST7789_DrawPoint(uint16_t x, uint16_t y, uint16_t color);
void     ST7789_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void     ST7789_WriteString(uint16_t x, uint16_t y, char *str, uint16_t color);
void     ST7789_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data);

/* 底层接口（高级用户可直接调用） */
void     ST7789_SetWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void     ST7789_WriteData(const uint8_t *data, uint32_t len);

#endif /* DEVICE_ST7789 */

#endif /* __ST7789_H */

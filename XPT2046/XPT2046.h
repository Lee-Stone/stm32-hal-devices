#ifndef __XPT2046_H
#define __XPT2046_H

#include "config.h"

#if DEVICE_XPT2046

#include <stdint.h>

/* ==================== 屏幕参数（运行时可变，配合旋转） ==================== */

extern uint16_t xpt2046_width;
extern uint16_t xpt2046_height;
#define XPT2046_WIDTH           (xpt2046_width)
#define XPT2046_HEIGHT          (xpt2046_height)

/* ==================== 校准参数（根据实际屏幕微调） ==================== */

#define XPT2046_X_MIN       300
#define XPT2046_X_MAX       3800
#define XPT2046_Y_MIN       300
#define XPT2046_Y_MAX       3800

/* ==================== API 函数声明 ==================== */

void     XPT2046_Init(void);
void     XPT2046_SetRotation(uint8_t rotation);
uint8_t  XPT2046_IsPressed(void);
uint8_t  XPT2046_ReadXY(uint16_t *x, uint16_t *y);

#endif /* DEVICE_XPT2046 */

#endif /* __XPT2046_H */

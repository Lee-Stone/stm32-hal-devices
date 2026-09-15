# Device 模块代码与文档模板

只在创建或移植驱动时读取本文件。模板中的占位符应替换为当前模块名称，具体硬件参数以用户指定的已运行工程为准。

## config.h

```c
/* ==================== XXXXX 配置 ==================== */

#define DEVICE_XXXXX  0
#if DEVICE_XXXXX
    #include "xxx.h"
    #define XXXXX_HANDLE    hxxx
#endif
```

硬件句柄和引脚映射放在此处；芯片地址、容量、块大小等模块固有参数放在模块头文件中。

## XXXXX.h

```c
#ifndef __XXXXX_H
#define __XXXXX_H

#include "config.h"

#if DEVICE_XXXXX

#include <stdint.h>

/* ==================== 参数宏定义 ==================== */

#define XXXXX_OK    0U

/* ==================== API 函数声明 ==================== */

uint8_t XXXXX_Init(void);                         // 初始化
uint8_t XXXXX_Read(uint8_t *data, uint32_t size); // 读取数据

#endif /* DEVICE_XXXXX */

#endif /* __XXXXX_H */
```

## XXXXX.c

```c
#include "XXXXX.h"

#if DEVICE_XXXXX

/* ==================== API 函数实现 ==================== */

/**
  * @brief  初始化模块
  * @param  无
  * @retval XXXXX_OK 初始化成功，其他值初始化失败
  */
uint8_t XXXXX_Init(void)
{
    return XXXXX_OK;
}

/**
  * @brief  读取数据
  * @param  data 接收缓冲区
  * @param  size 读取字节数
  * @retval XXXXX_OK 读取成功，其他值读取失败
  */
uint8_t XXXXX_Read(uint8_t *data, uint32_t size)
{
    /* 实现 */
}

#endif /* DEVICE_XXXXX */
```

每个自有函数写完整的 `@brief`、`@param` 和 `@retval`。无参数写“无”，无返回值写“无”。第三方源码保持上游格式。

## README 模块章节

```markdown
### N. XXXXX 类型模块

[一句话说明支持的器件、主要功能和通信方式。]

![图片编号](images/图片编号.png)

#### 硬件连接

| 模块引脚 | STM32 引脚 | 说明 |
|---------|-----------|------|
| VCC | 3.3V | 电源 |
| GND | GND | 公共地 |

#### CubeMX 配置

**添加路径：**

- 在包含路径和源位置中添加 `Devices/XXXXX`。

![图片编号](images/图片编号.png)

**外设配置：**

- 写出参考工程中的接口模式、引脚、时钟、分频、上下拉和其他关键参数。
- 其他选项保持参考工程配置。

![图片编号](images/图片编号.png)

#### config.h 配置

```c
#define DEVICE_XXXXX  1
#if DEVICE_XXXXX
    #include "xxx.h"
    #define XXXXX_HANDLE    hxxx
#endif
```

#### API 接口

```c
uint8_t XXXXX_Init(void);                         // 初始化
uint8_t XXXXX_Read(uint8_t *data, uint32_t size); // 读取数据
```

#### 使用示例

```c
#include "XXXXX.h"

int main(void)
{
    XXXXX_Init();

    while (1)
    {
        HAL_Delay(100U);
    }
}
```

---
```

图片只写占位路径。编号沿用 README 中最近新增模块的实际规则，不创建空 PNG 文件。

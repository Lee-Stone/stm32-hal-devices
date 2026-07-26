#include "AT24CXX.h"

#if DEVICE_AT24CXX

/**
  * @brief  AT24CXX 初始化，检测设备是否存在
  * @param  无
  * @retval 无
  */
void AT24CXX_Init(void)
{
    /* 发送一个假写操作检测设备是否在 I2C 总线上应答 */
    HAL_I2C_IsDeviceReady(&AT24CXX_I2C, AT24CXX_DEVICE_ADDR, 3, HAL_MAX_DELAY);
}

/**
  * @brief  写入单个字节
  * @param  addr 内存地址（0 ~ MEM_SIZE-1）
  * @param  data 要写入的数据
  * @retval 无
  */
void AT24CXX_WriteByte(uint16_t addr, uint8_t data)
{
    HAL_I2C_Mem_Write(&AT24CXX_I2C, AT24CXX_DEVICE_ADDR,
                      addr, AT24CXX_ADDR_SIZE,
                      &data, 1, HAL_MAX_DELAY);
    HAL_Delay(5);   /* 等待内部写周期完成（tWR ≤ 5ms） */
}

/**
  * @brief  读取单个字节
  * @param  addr 内存地址（0 ~ MEM_SIZE-1）
  * @retval 读取到的数据
  */
uint8_t AT24CXX_ReadByte(uint16_t addr)
{
    uint8_t data;
    HAL_I2C_Mem_Read(&AT24CXX_I2C, AT24CXX_DEVICE_ADDR,
                     addr, AT24CXX_ADDR_SIZE,
                     &data, 1, HAL_MAX_DELAY);
    return data;
}

/**
  * @brief  写入多个字节（自动处理页边界与跨页）
  * @param  addr 起始内存地址
  * @param  data 数据缓冲区指针
  * @param  len  要写入的字节数
  * @retval 无
  */
void AT24CXX_Write(uint16_t addr, uint8_t *data, uint16_t len)
{
    uint16_t remain = len;
    uint16_t offset = 0;

    while (remain > 0)
    {
        /* 计算当前页剩余空间 */
        uint16_t page_remain = AT24CXX_PAGE_SIZE - (addr % AT24CXX_PAGE_SIZE);
        uint16_t chunk = (remain < page_remain) ? remain : page_remain;

        HAL_I2C_Mem_Write(&AT24CXX_I2C, AT24CXX_DEVICE_ADDR,
                          addr, AT24CXX_ADDR_SIZE,
                          data + offset, chunk, HAL_MAX_DELAY);
        HAL_Delay(5);   /* 等待内部写周期完成 */

        addr   += chunk;
        offset += chunk;
        remain -= chunk;
    }
}

/**
  * @brief  读取多个字节（连续读取，无页限制）
  * @param  addr 起始内存地址
  * @param  data 数据缓冲区指针
  * @param  len  要读取的字节数
  * @retval 无
  */
void AT24CXX_Read(uint16_t addr, uint8_t *data, uint16_t len)
{
    HAL_I2C_Mem_Read(&AT24CXX_I2C, AT24CXX_DEVICE_ADDR,
                     addr, AT24CXX_ADDR_SIZE,
                     data, len, HAL_MAX_DELAY);
}

/**
  * @brief  全片擦除（所有字节写为 0xFF）
  * @param  无
  * @retval 无
  */
void AT24CXX_EraseChip(void)
{
    uint8_t buf[AT24CXX_PAGE_SIZE];
    uint16_t i;

    for (i = 0; i < AT24CXX_PAGE_SIZE; i++)
    {
        buf[i] = 0xFF;
    }

    for (i = 0; i < AT24CXX_MEM_SIZE; i += AT24CXX_PAGE_SIZE)
    {
        AT24CXX_Write(i, buf, AT24CXX_PAGE_SIZE);
    }
}

#endif /* DEVICE_AT24CXX */

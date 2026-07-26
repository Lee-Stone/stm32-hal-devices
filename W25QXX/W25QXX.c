#include "W25QXX.h"

#if DEVICE_W25QXX

/* ==================== 私有辅助宏 ==================== */

#define W25QXX_DUMMY_BYTE               0xFF

/* CS 低有效：选中芯片 */
#define W25QXX_CS_L()                   W25QXX_CS(0)
/* CS 高有效：释放芯片 */
#define W25QXX_CS_H()                   W25QXX_CS(1)

/* ==================== 私有函数声明 ==================== */

static void    w25qxx_spi_tx(uint8_t *data, uint16_t len);
static void    w25qxx_spi_rx(uint8_t *data, uint16_t len);
static void    w25qxx_write_enable(void);
static void    w25qxx_wait_busy(void);
static uint8_t w25qxx_read_sr1(void);

/* ==================== SPI 底层收发 ==================== */

/**
  * @brief  SPI 发送数据
  * @param  data 数据缓冲区
  * @param  len  发送字节数
  * @retval 无
  */
static void w25qxx_spi_tx(uint8_t *data, uint16_t len)
{
    W25QXX_CS_L();
    HAL_SPI_Transmit(&W25QXX_SPI, data, len, HAL_MAX_DELAY);
    W25QXX_CS_H();
}

/**
  * @brief  SPI 接收数据
  * @param  data 数据缓冲区
  * @param  len  接收字节数
  * @retval 无
  */
static void w25qxx_spi_rx(uint8_t *data, uint16_t len)
{
    W25QXX_CS_L();
    HAL_SPI_Receive(&W25QXX_SPI, data, len, HAL_MAX_DELAY);
    W25QXX_CS_H();
}

/**
  * @brief  发送单字节指令（无地址、无数据）
  * @param  cmd 指令字节
  * @retval 无
  */
static void w25qxx_send_cmd(uint8_t cmd)
{
    W25QXX_CS_L();
    HAL_SPI_Transmit(&W25QXX_SPI, &cmd, 1, HAL_MAX_DELAY);
    W25QXX_CS_H();
}

/* ==================== 状态与控制 ==================== */

/**
  * @brief  写使能（执行擦除/编程前必须调用）
  * @param  无
  * @retval 无
  */
static void w25qxx_write_enable(void)
{
    w25qxx_send_cmd(W25QXX_CMD_WREN);
}

/**
  * @brief  读状态寄存器 1
  * @param  无
  * @retval SR1 的值
  */
static uint8_t w25qxx_read_sr1(void)
{
    uint8_t cmd = W25QXX_CMD_RDSR1;
    uint8_t sr  = 0;

    W25QXX_CS_L();
    HAL_SPI_Transmit(&W25QXX_SPI, &cmd, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(&W25QXX_SPI, &sr, 1, HAL_MAX_DELAY);
    W25QXX_CS_H();

    return sr;
}

/**
  * @brief  等待 BUSY 位清零（擦除/编程完成后才能继续操作）
  * @param  无
  * @retval 无
  */
static void w25qxx_wait_busy(void)
{
    while (w25qxx_read_sr1() & 0x01)
    {
        /* 等待 WIP（Write In Progress）位清零 */
    }
}

/* ==================== 初始化与识别 ==================== */

/**
  * @brief  W25QXX 初始化
  * @param  无
  * @retval 无
  */
void W25QXX_Init(void)
{
    W25QXX_CS_H();      /* 初始状态：不选中 */
}

/**
  * @brief  读取 JEDEC ID（制造商 ID + 设备 ID）
  * @param  mid 制造商 ID 指针（0xEF = Winbond）
  * @param  did 设备 ID 指针
  * @retval 无
  */
void W25QXX_ReadID(uint8_t *mid, uint16_t *did)
{
    uint8_t cmd = W25QXX_CMD_JEDEC_ID;
    uint8_t buf[3] = {0};

    W25QXX_CS_L();
    HAL_SPI_Transmit(&W25QXX_SPI, &cmd, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(&W25QXX_SPI, buf, 3, HAL_MAX_DELAY);
    W25QXX_CS_H();

    *mid = buf[0];
    *did = ((uint16_t)buf[1] << 8) | buf[2];
}

/* ==================== 读操作 ==================== */

/**
  * @brief  读取数据（标准读，最大支持 16MB 地址空间）
  * @param  addr 起始地址（0 ~ MEM_SIZE-1）
  * @param  data 数据缓冲区指针
  * @param  len  读取字节数
  * @retval 无
  */
void W25QXX_Read(uint32_t addr, uint8_t *data, uint32_t len)
{
    uint8_t cmd[4];

    cmd[0] = W25QXX_CMD_READ;
    cmd[1] = (uint8_t)(addr >> 16);
    cmd[2] = (uint8_t)(addr >> 8);
    cmd[3] = (uint8_t)(addr);

    W25QXX_CS_L();
    HAL_SPI_Transmit(&W25QXX_SPI, cmd, 4, HAL_MAX_DELAY);
    HAL_SPI_Receive(&W25QXX_SPI, data, (uint16_t)len, HAL_MAX_DELAY);
    W25QXX_CS_H();
}

/* ==================== 编程与擦除 ==================== */

/**
  * @brief  页编程（单页内写入，≤256 字节，不可跨页）
  * @param  addr 页内起始地址
  * @param  data 数据缓冲区指针
  * @param  len  写入字节数（≤ 256，且不能跨页边界）
  * @retval 无
  */
void W25QXX_PageProgram(uint32_t addr, uint8_t *data, uint16_t len)
{
    uint8_t cmd[4];

    w25qxx_write_enable();

    cmd[0] = W25QXX_CMD_PAGE_PROG;
    cmd[1] = (uint8_t)(addr >> 16);
    cmd[2] = (uint8_t)(addr >> 8);
    cmd[3] = (uint8_t)(addr);

    W25QXX_CS_L();
    HAL_SPI_Transmit(&W25QXX_SPI, cmd, 4, HAL_MAX_DELAY);
    HAL_SPI_Transmit(&W25QXX_SPI, data, len, HAL_MAX_DELAY);
    W25QXX_CS_H();

    w25qxx_wait_busy();
}

/**
  * @brief  写入多个字节（自动处理扇区擦除与跨页）
  * @param  addr 起始地址
  * @param  data 数据缓冲区指针
  * @param  len  写入字节数
  * @retval 无
  */
void W25QXX_Write(uint32_t addr, uint8_t *data, uint32_t len)
{
    uint32_t remain = len;
    uint32_t offset = 0;

    while (remain > 0)
    {
        /* 擦除当前扇区（仅在扇区起始地址时执行） */
        if ((addr % W25QXX_SECTOR_SIZE) == 0)
        {
            W25QXX_SectorErase(addr);
        }

        /* 计算当前页剩余空间 */
        uint32_t page_remain = W25QXX_PAGE_SIZE - (addr % W25QXX_PAGE_SIZE);
        uint32_t chunk = (remain < page_remain) ? remain : page_remain;

        W25QXX_PageProgram(addr, data + offset, (uint16_t)chunk);

        addr   += chunk;
        offset += chunk;
        remain -= chunk;
    }
}

/**
  * @brief  扇区擦除（4KB）
  * @param  addr 扇区起始地址（必须 4KB 对齐）
  * @retval 无
  */
void W25QXX_SectorErase(uint32_t addr)
{
    uint8_t cmd[4];

    w25qxx_write_enable();

    cmd[0] = W25QXX_CMD_SECTOR_ERASE;
    cmd[1] = (uint8_t)(addr >> 16);
    cmd[2] = (uint8_t)(addr >> 8);
    cmd[3] = (uint8_t)(addr);

    W25QXX_CS_L();
    HAL_SPI_Transmit(&W25QXX_SPI, cmd, 4, HAL_MAX_DELAY);
    W25QXX_CS_H();

    w25qxx_wait_busy();
}

/**
  * @brief  块擦除（32KB）
  * @param  addr 块起始地址
  * @retval 无
  */
void W25QXX_BlockErase32K(uint32_t addr)
{
    uint8_t cmd[4];

    w25qxx_write_enable();

    cmd[0] = W25QXX_CMD_BLOCK_ERASE_32K;
    cmd[1] = (uint8_t)(addr >> 16);
    cmd[2] = (uint8_t)(addr >> 8);
    cmd[3] = (uint8_t)(addr);

    W25QXX_CS_L();
    HAL_SPI_Transmit(&W25QXX_SPI, cmd, 4, HAL_MAX_DELAY);
    W25QXX_CS_H();

    w25qxx_wait_busy();
}

/**
  * @brief  块擦除（64KB）
  * @param  addr 块起始地址
  * @retval 无
  */
void W25QXX_BlockErase64K(uint32_t addr)
{
    uint8_t cmd[4];

    w25qxx_write_enable();

    cmd[0] = W25QXX_CMD_BLOCK_ERASE_64K;
    cmd[1] = (uint8_t)(addr >> 16);
    cmd[2] = (uint8_t)(addr >> 8);
    cmd[3] = (uint8_t)(addr);

    W25QXX_CS_L();
    HAL_SPI_Transmit(&W25QXX_SPI, cmd, 4, HAL_MAX_DELAY);
    W25QXX_CS_H();

    w25qxx_wait_busy();
}

/**
  * @brief  整片擦除
  * @param  无
  * @retval 无
  */
void W25QXX_ChipErase(void)
{
    w25qxx_write_enable();
    w25qxx_send_cmd(W25QXX_CMD_CHIP_ERASE);
    w25qxx_wait_busy();
}

#endif /* DEVICE_W25QXX */

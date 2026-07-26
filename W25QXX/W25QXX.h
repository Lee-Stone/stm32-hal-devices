#ifndef __W25QXX_H
#define __W25QXX_H

#include "config.h"

#if DEVICE_W25QXX

#include <stdint.h>

/* ==================== W25QXX 芯片型号宏定义 ==================== */

#define W25Q16         0   /*  16Mbit =   2MB */
#define W25Q32         1   /*  32Mbit =   4MB */
#define W25Q64         2   /*  64Mbit =   8MB */
#define W25Q128        3   /* 128Mbit =  16MB */
#define W25Q256        4   /* 256Mbit =  32MB */

/* ==================== SPI 指令表 ==================== */

#define W25QXX_CMD_WREN              0x06  /* 写使能                   */
#define W25QXX_CMD_WRDI              0x04  /* 写禁止                   */
#define W25QXX_CMD_RDSR1             0x05  /* 读状态寄存器 1            */
#define W25QXX_CMD_RDSR2             0x35  /* 读状态寄存器 2            */
#define W25QXX_CMD_READ              0x03  /* 读数据（标准）            */
#define W25QXX_CMD_FAST_READ         0x0B  /* 读数据（快速）            */
#define W25QXX_CMD_PAGE_PROG         0x02  /* 页编程（≤256 字节）       */
#define W25QXX_CMD_SECTOR_ERASE      0x20  /* 扇区擦除（4KB）           */
#define W25QXX_CMD_BLOCK_ERASE_32K   0x52  /* 块擦除（32KB）            */
#define W25QXX_CMD_BLOCK_ERASE_64K   0xD8  /* 块擦除（64KB）            */
#define W25QXX_CMD_CHIP_ERASE        0xC7  /* 整片擦除                  */
#define W25QXX_CMD_POWER_DOWN        0xB9  /* 掉电模式                  */
#define W25QXX_CMD_RELEASE_PD        0xAB  /* 唤醒                      */
#define W25QXX_CMD_JEDEC_ID          0x9F  /* 读 JEDEC ID               */

/* ==================== 根据芯片型号计算参数 ==================== */

#if W25QXX_CHIP == W25Q16
    #define W25QXX_MEM_SIZE             (2UL * 1024UL * 1024UL)   /* 2MB   */
    #define W25QXX_SECTOR_COUNT         512U
    #define W25QXX_BLOCK_64K_COUNT      32U
#elif W25QXX_CHIP == W25Q32
    #define W25QXX_MEM_SIZE             (4UL * 1024UL * 1024UL)   /* 4MB   */
    #define W25QXX_SECTOR_COUNT         1024U
    #define W25QXX_BLOCK_64K_COUNT      64U
#elif W25QXX_CHIP == W25Q64
    #define W25QXX_MEM_SIZE             (8UL * 1024UL * 1024UL)   /* 8MB   */
    #define W25QXX_SECTOR_COUNT         2048U
    #define W25QXX_BLOCK_64K_COUNT      128U
#elif W25QXX_CHIP == W25Q128
    #define W25QXX_MEM_SIZE             (16UL * 1024UL * 1024UL)  /* 16MB  */
    #define W25QXX_SECTOR_COUNT         4096U
    #define W25QXX_BLOCK_64K_COUNT      256U
#elif W25QXX_CHIP == W25Q256
    #define W25QXX_MEM_SIZE             (32UL * 1024UL * 1024UL)  /* 32MB  */
    #define W25QXX_SECTOR_COUNT         8192U
    #define W25QXX_BLOCK_64K_COUNT      512U
#else
    #error "W25QXX_CHIP 未定义或值无效！请在 config.h 中设置 W25QXX_CHIP"
#endif

/* 以下参数所有型号相同 */
#define W25QXX_PAGE_SIZE                256U
#define W25QXX_SECTOR_SIZE              4096U
#define W25QXX_BLOCK_32K_SIZE           32768U
#define W25QXX_BLOCK_64K_SIZE           65536U

/* ==================== API 函数声明 ==================== */

void     W25QXX_Init(void);
void     W25QXX_ReadID(uint8_t *mid, uint16_t *did);
void     W25QXX_Read(uint32_t addr, uint8_t *data, uint32_t len);
void     W25QXX_PageProgram(uint32_t addr, uint8_t *data, uint16_t len);
void     W25QXX_Write(uint32_t addr, uint8_t *data, uint32_t len);
void     W25QXX_SectorErase(uint32_t addr);
void     W25QXX_BlockErase32K(uint32_t addr);
void     W25QXX_BlockErase64K(uint32_t addr);
void     W25QXX_ChipErase(void);

#endif /* DEVICE_W25QXX */

#endif /* __W25QXX_H */

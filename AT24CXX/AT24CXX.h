#ifndef __AT24CXX_H
#define __AT24CXX_H

#include "config.h"

#if DEVICE_AT24CXX

#include <stdint.h>

/* ==================== AT24CXX 芯片型号宏定义 ==================== */

#define AT24C01         0   /*   1Kbit =   128 字节 */
#define AT24C02         1   /*   2Kbit =   256 字节 */
#define AT24C04         2   /*   4Kbit =   512 字节 */
#define AT24C08         3   /*   8Kbit =  1024 字节 */
#define AT24C16         4   /*  16Kbit =  2048 字节 */
#define AT24C32         5   /*  32Kbit =  4096 字节 */
#define AT24C64         6   /*  64Kbit =  8192 字节 */
#define AT24C128        7   /* 128Kbit = 16384 字节 */
#define AT24C256        8   /* 256Kbit = 32768 字节 */
#define AT24C512        9   /* 512Kbit = 65536 字节 */

/* ==================== 根据芯片型号计算参数 ==================== */

#if AT24CXX_CHIP == AT24C01
    #define AT24CXX_MEM_SIZE        128U
    #define AT24CXX_PAGE_SIZE       8U
    #define AT24CXX_ADDR_SIZE       I2C_MEMADD_SIZE_8BIT
    #define AT24CXX_DEVICE_ADDR     0xA0
#elif AT24CXX_CHIP == AT24C02
    #define AT24CXX_MEM_SIZE        256U
    #define AT24CXX_PAGE_SIZE       8U
    #define AT24CXX_ADDR_SIZE       I2C_MEMADD_SIZE_8BIT
    #define AT24CXX_DEVICE_ADDR     0xA0
#elif AT24CXX_CHIP == AT24C04
    #define AT24CXX_MEM_SIZE        512U
    #define AT24CXX_PAGE_SIZE       16U
    #define AT24CXX_ADDR_SIZE       I2C_MEMADD_SIZE_8BIT
    #define AT24CXX_DEVICE_ADDR     0xA0
#elif AT24CXX_CHIP == AT24C08
    #define AT24CXX_MEM_SIZE        1024U
    #define AT24CXX_PAGE_SIZE       16U
    #define AT24CXX_ADDR_SIZE       I2C_MEMADD_SIZE_8BIT
    #define AT24CXX_DEVICE_ADDR     0xA0
#elif AT24CXX_CHIP == AT24C16
    #define AT24CXX_MEM_SIZE        2048U
    #define AT24CXX_PAGE_SIZE       16U
    #define AT24CXX_ADDR_SIZE       I2C_MEMADD_SIZE_8BIT
    #define AT24CXX_DEVICE_ADDR     0xA0
#elif AT24CXX_CHIP == AT24C32
    #define AT24CXX_MEM_SIZE        4096U
    #define AT24CXX_PAGE_SIZE       32U
    #define AT24CXX_ADDR_SIZE       I2C_MEMADD_SIZE_16BIT
    #define AT24CXX_DEVICE_ADDR     0xA0
#elif AT24CXX_CHIP == AT24C64
    #define AT24CXX_MEM_SIZE        8192U
    #define AT24CXX_PAGE_SIZE       32U
    #define AT24CXX_ADDR_SIZE       I2C_MEMADD_SIZE_16BIT
    #define AT24CXX_DEVICE_ADDR     0xA0
#elif AT24CXX_CHIP == AT24C128
    #define AT24CXX_MEM_SIZE        16384U
    #define AT24CXX_PAGE_SIZE       64U
    #define AT24CXX_ADDR_SIZE       I2C_MEMADD_SIZE_16BIT
    #define AT24CXX_DEVICE_ADDR     0xA0
#elif AT24CXX_CHIP == AT24C256
    #define AT24CXX_MEM_SIZE        32768U
    #define AT24CXX_PAGE_SIZE       64U
    #define AT24CXX_ADDR_SIZE       I2C_MEMADD_SIZE_16BIT
    #define AT24CXX_DEVICE_ADDR     0xA0
#elif AT24CXX_CHIP == AT24C512
    #define AT24CXX_MEM_SIZE        65536U
    #define AT24CXX_PAGE_SIZE       128U
    #define AT24CXX_ADDR_SIZE       I2C_MEMADD_SIZE_16BIT
    #define AT24CXX_DEVICE_ADDR     0xA0
#else
    #error "AT24CXX_CHIP 未定义或值无效！请在 config.h 中设置 AT24CXX_CHIP"
#endif

/* ==================== API 函数声明 ==================== */

void     AT24CXX_Init(void);
void     AT24CXX_WriteByte(uint16_t addr, uint8_t data);
uint8_t  AT24CXX_ReadByte(uint16_t addr);
void     AT24CXX_Write(uint16_t addr, uint8_t *data, uint16_t len);
void     AT24CXX_Read(uint16_t addr, uint8_t *data, uint16_t len);
void     AT24CXX_EraseChip(void);

#endif /* DEVICE_AT24CXX */

#endif /* __AT24CXX_H */

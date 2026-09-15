#ifndef __SDCARD_H
#define __SDCARD_H

#include "config.h"

#if DEVICE_SDCARD

#include "ff.h"
#include <stdint.h>

/* ==================== SD 卡参数宏定义 ==================== */

#define SDCARD_BLOCK_SIZE             512U
#define SDCARD_DEFAULT_TIMEOUT_MS    5000U
#define SDCARD_MAX_TRANSFER_BLOCKS    128U
#define SDCARD_POWER_UP_DELAY_MS        50U
#define SDCARD_INIT_RETRY_DELAY_MS      20U
#define SDCARD_INIT_RETRY_COUNT          3U

/* ==================== 类型定义 ==================== */

typedef enum
{
    SDCARD_OK = 0,
    SDCARD_ERROR_ARGUMENT,
    SDCARD_ERROR_NOT_INITIALIZED,
    SDCARD_ERROR_NOT_READY,
    SDCARD_ERROR_BLOCK_SIZE,
    SDCARD_ERROR_RANGE,
    SDCARD_ERROR_HAL,
    SDCARD_ERROR_TIMEOUT
} SDCard_Status_t;

typedef enum
{
    SDCARD_INIT_STAGE_IDLE = 0,
    SDCARD_INIT_STAGE_HAL_INIT,
    SDCARD_INIT_STAGE_WIDE_BUS,
    SDCARD_INIT_STAGE_CARD_INFO,
    SDCARD_INIT_STAGE_WAIT_READY,
    SDCARD_INIT_STAGE_READY
} SDCard_InitStage_t;

typedef struct
{
    uint32_t card_type;
    uint32_t block_size;
    uint32_t block_count;
    uint64_t capacity_bytes;
} SDCard_Info_t;

/* ==================== API 函数声明 ==================== */

FRESULT         SDCard_WriteFile(const char *path, const void *data,
                                 uint32_t size, uint32_t *bytes_written);              // 创建或覆盖文件并写入数据
FRESULT         SDCard_ReadFile(const char *path, void *data,
                                uint32_t size, uint32_t *bytes_read);                  // 读取文件数据
FRESULT         SDCard_Mount(void);                                                     // 挂载 FAT 文件系统
FRESULT         SDCard_Unmount(void);                                                   // 卸载 FAT 文件系统
uint8_t         SDCard_IsMounted(void);                                                 // 查询文件系统挂载状态
FRESULT         SDCard_RemoveFile(const char *path);                                    // 删除文件
FRESULT         SDCard_GetSpace(uint32_t *total_kb, uint32_t *free_kb);                 // 获取总容量和剩余容量

SDCard_Status_t SDCard_Init(void);                                                       // 初始化 SDIO 和 SD 卡
SDCard_Status_t SDCard_DeInit(void);                                                     // 关闭 SD 卡外设
SDCard_Status_t SDCard_GetInfo(SDCard_Info_t *info);                                    // 获取容量和块信息
SDCard_Status_t SDCard_ReadBlocks(uint32_t block, uint8_t *data,
                                  uint32_t block_count);                                 // 读取连续逻辑块
SDCard_Status_t SDCard_WriteBlocks(uint32_t block, const uint8_t *data,
                                   uint32_t block_count);                                // 写入连续逻辑块
SDCard_Status_t SDCard_EraseBlocks(uint32_t first_block, uint32_t last_block);           // 擦除指定逻辑块范围
SDCard_Status_t SDCard_Sync(void);                                                       // 等待卡完成内部操作
uint8_t         SDCard_IsReady(void);                                                    // 查询卡是否可传输
uint32_t        SDCard_GetHalError(void);                                                // 获取最近一次 HAL SD 错误位
const char     *SDCard_GetStatusText(SDCard_Status_t status);                            // 获取驱动状态文字
SDCard_InitStage_t SDCard_GetInitStage(void);                                            // 获取初始化停止阶段
const char     *SDCard_GetInitStageText(SDCard_InitStage_t stage);                       // 获取初始化阶段文字

SDCard_Status_t SDCard_Test_Init(void);                                                  // 初始化 SD 卡测试
void            SDCard_Test_Poll(void);                                                  // SD 卡测试轮询入口

#endif /* DEVICE_SDCARD */

#endif /* __SDCARD_H */

#include "SDCard.h"

#if DEVICE_SDCARD

#include <stdio.h>
#include <string.h>

/* ==================== 内部变量 ==================== */

static uint8_t SDCard_Initialized;
static uint32_t SDCard_LastHalError;
static SDCard_InitStage_t SDCard_InitStage;
static uint8_t SDCard_TestBuffer[SDCARD_BLOCK_SIZE];
static FATFS SDCard_FileSystem;
static FIL SDCard_File;
static uint8_t SDCard_Mounted;

/* ==================== 内部函数 ==================== */

/**
  * @brief  等待 SD 卡进入可传输状态
  * @param  timeout_ms 最长等待时间，单位毫秒
  * @retval SDCard_Status_t 等待结果
  */
static SDCard_Status_t SDCard_WaitReady(uint32_t timeout_ms)
{
    uint32_t start_tick = HAL_GetTick();

    for (;;)
    {
        HAL_SD_CardStateTypeDef state = HAL_SD_GetCardState(&SDCARD_HANDLE);

        if (state == HAL_SD_CARD_TRANSFER)
        {
            return SDCARD_OK;
        }
        if ((state != HAL_SD_CARD_SENDING) &&
            (state != HAL_SD_CARD_RECEIVING) &&
            (state != HAL_SD_CARD_PROGRAMMING))
        {
            return SDCARD_ERROR_NOT_READY;
        }
        if ((HAL_GetTick() - start_tick) >= timeout_ms)
        {
            return SDCARD_ERROR_TIMEOUT;
        }
    }
}

/**
  * @brief  检查块范围是否合法
  * @param  block 起始逻辑块号
  * @param  block_count 逻辑块数量
  * @retval SDCard_Status_t 检查结果
  */
static SDCard_Status_t SDCard_CheckRange(uint32_t block, uint32_t block_count)
{
    uint32_t total_blocks = SDCARD_HANDLE.SdCard.LogBlockNbr;

    if (block_count == 0U)
    {
        return SDCARD_ERROR_ARGUMENT;
    }
    if ((block >= total_blocks) || (block_count > (total_blocks - block)))
    {
        return SDCARD_ERROR_RANGE;
    }
    return SDCARD_OK;
}

/* ==================== API 函数 ==================== */

/**
  * @brief  初始化 SDIO 外设和 SD 卡
  * @param  无
  * @retval SDCard_Status_t 初始化结果
  */
SDCard_Status_t SDCard_Init(void)
{
    HAL_SD_CardInfoTypeDef card_info;
    uint32_t attempt;
    HAL_StatusTypeDef hal_status = HAL_ERROR;

    if ((SDCard_Initialized != 0U) && (SDCard_IsReady() != 0U))
    {
        return SDCARD_OK;
    }

    SDCard_Initialized = 0U;
    SDCard_LastHalError = HAL_SD_ERROR_NONE;
    SDCard_InitStage = SDCARD_INIT_STAGE_HAL_INIT;
    HAL_Delay(SDCARD_POWER_UP_DELAY_MS);

    for (attempt = 0U; attempt < SDCARD_INIT_RETRY_COUNT; attempt++)
    {
        if ((SDCARD_HANDLE.Instance == SDCARD_INSTANCE) &&
            (HAL_SD_GetState(&SDCARD_HANDLE) != HAL_SD_STATE_RESET))
        {
            (void)HAL_SD_DeInit(&SDCARD_HANDLE);
        }

        SDCARD_HANDLE.Instance = SDCARD_INSTANCE;
        SDCARD_HANDLE.Init.ClockEdge = SDCARD_CLOCK_EDGE;
        SDCARD_HANDLE.Init.ClockBypass = SDCARD_CLOCK_BYPASS;
        SDCARD_HANDLE.Init.ClockPowerSave = SDCARD_CLOCK_POWER_SAVE;
        SDCARD_HANDLE.Init.BusWide = SDIO_BUS_WIDE_1B;
        SDCARD_HANDLE.Init.HardwareFlowControl = SDCARD_HARDWARE_FLOW;
        SDCARD_HANDLE.Init.ClockDiv = SDCARD_CLOCK_DIVIDER;

        hal_status = HAL_SD_Init(&SDCARD_HANDLE);
        SDCard_LastHalError = HAL_SD_GetError(&SDCARD_HANDLE);
        if (hal_status == HAL_OK)
        {
            break;
        }
        (void)HAL_SD_DeInit(&SDCARD_HANDLE);
        HAL_Delay(SDCARD_INIT_RETRY_DELAY_MS);
    }
    if (hal_status != HAL_OK)
    {
        return SDCARD_ERROR_HAL;
    }

    SDCard_InitStage = SDCARD_INIT_STAGE_WIDE_BUS;
    if (HAL_SD_ConfigWideBusOperation(&SDCARD_HANDLE,
                                      SDCARD_BUS_WIDTH) != HAL_OK)
    {
        SDCard_LastHalError = HAL_SD_GetError(&SDCARD_HANDLE);
        (void)HAL_SD_DeInit(&SDCARD_HANDLE);
        return SDCARD_ERROR_HAL;
    }

    SDCard_InitStage = SDCARD_INIT_STAGE_CARD_INFO;
    if (HAL_SD_GetCardInfo(&SDCARD_HANDLE, &card_info) != HAL_OK)
    {
        SDCard_LastHalError = HAL_SD_GetError(&SDCARD_HANDLE);
        return SDCARD_ERROR_HAL;
    }
    if (card_info.LogBlockSize != SDCARD_BLOCK_SIZE)
    {
        SDCard_Initialized = 0U;
        return SDCARD_ERROR_BLOCK_SIZE;
    }
    SDCard_InitStage = SDCARD_INIT_STAGE_WAIT_READY;
    if (SDCard_WaitReady(SDCARD_DEFAULT_TIMEOUT_MS) != SDCARD_OK)
    {
        SDCard_LastHalError = HAL_SD_GetError(&SDCARD_HANDLE);
        return SDCARD_ERROR_NOT_READY;
    }

    SDCard_Initialized = 1U;
    SDCard_InitStage = SDCARD_INIT_STAGE_READY;
    return SDCARD_OK;
}

/**
  * @brief  关闭 SD 卡外设
  * @param  无
  * @retval SDCard_Status_t 关闭结果
  */
SDCard_Status_t SDCard_DeInit(void)
{
    if (SDCard_Mounted != 0U)
    {
        (void)f_mount(NULL, "0:", 0U);
        SDCard_Mounted = 0U;
    }
    if (HAL_SD_DeInit(&SDCARD_HANDLE) != HAL_OK)
    {
        SDCard_LastHalError = HAL_SD_GetError(&SDCARD_HANDLE);
        return SDCARD_ERROR_HAL;
    }
    SDCard_Initialized = 0U;
    return SDCARD_OK;
}

/**
  * @brief  获取 SD 卡容量和逻辑块信息
  * @param  info SD 卡信息输出结构体
  * @retval SDCard_Status_t 读取结果
  */
SDCard_Status_t SDCard_GetInfo(SDCard_Info_t *info)
{
    HAL_SD_CardInfoTypeDef card_info;

    if (info == NULL)
    {
        return SDCARD_ERROR_ARGUMENT;
    }
    if (SDCard_Initialized == 0U)
    {
        return SDCARD_ERROR_NOT_INITIALIZED;
    }
    if (HAL_SD_GetCardInfo(&SDCARD_HANDLE, &card_info) != HAL_OK)
    {
        SDCard_LastHalError = HAL_SD_GetError(&SDCARD_HANDLE);
        return SDCARD_ERROR_HAL;
    }

    info->card_type = card_info.CardType;
    info->block_size = card_info.LogBlockSize;
    info->block_count = card_info.LogBlockNbr;
    info->capacity_bytes = (uint64_t)card_info.LogBlockSize * card_info.LogBlockNbr;
    return SDCARD_OK;
}

/**
  * @brief  读取一个或多个连续逻辑块
  * @param  block 起始逻辑块号
  * @param  data 接收缓冲区
  * @param  block_count 读取的逻辑块数量
  * @retval SDCard_Status_t 读取结果
  */
SDCard_Status_t SDCard_ReadBlocks(uint32_t block, uint8_t *data, uint32_t block_count)
{
    SDCard_Status_t status;

    if (data == NULL)
    {
        return SDCARD_ERROR_ARGUMENT;
    }
    if (SDCard_Initialized == 0U)
    {
        return SDCARD_ERROR_NOT_INITIALIZED;
    }
    status = SDCard_CheckRange(block, block_count);
    if (status != SDCARD_OK)
    {
        return status;
    }

    while (block_count != 0U)
    {
        uint32_t transfer_blocks = (block_count > SDCARD_MAX_TRANSFER_BLOCKS) ?
                                   SDCARD_MAX_TRANSFER_BLOCKS : block_count;

        if (HAL_SD_ReadBlocks(&SDCARD_HANDLE, data, block, transfer_blocks,
                              SDCARD_DEFAULT_TIMEOUT_MS) != HAL_OK)
        {
            SDCard_LastHalError = HAL_SD_GetError(&SDCARD_HANDLE);
            return SDCARD_ERROR_HAL;
        }
        status = SDCard_WaitReady(SDCARD_DEFAULT_TIMEOUT_MS);
        if (status != SDCARD_OK)
        {
            return status;
        }
        block += transfer_blocks;
        block_count -= transfer_blocks;
        data += transfer_blocks * SDCARD_BLOCK_SIZE;
    }
    return SDCARD_OK;
}

/**
  * @brief  写入一个或多个连续逻辑块
  * @param  block 起始逻辑块号
  * @param  data 待写入缓冲区
  * @param  block_count 写入的逻辑块数量
  * @retval SDCard_Status_t 写入结果
  */
SDCard_Status_t SDCard_WriteBlocks(uint32_t block, const uint8_t *data,
                                   uint32_t block_count)
{
    SDCard_Status_t status;

    if (data == NULL)
    {
        return SDCARD_ERROR_ARGUMENT;
    }
    if (SDCard_Initialized == 0U)
    {
        return SDCARD_ERROR_NOT_INITIALIZED;
    }
    status = SDCard_CheckRange(block, block_count);
    if (status != SDCARD_OK)
    {
        return status;
    }

    while (block_count != 0U)
    {
        uint32_t transfer_blocks = (block_count > SDCARD_MAX_TRANSFER_BLOCKS) ?
                                   SDCARD_MAX_TRANSFER_BLOCKS : block_count;

        if (HAL_SD_WriteBlocks(&SDCARD_HANDLE, (uint8_t *)data, block,
                               transfer_blocks, SDCARD_DEFAULT_TIMEOUT_MS) != HAL_OK)
        {
            SDCard_LastHalError = HAL_SD_GetError(&SDCARD_HANDLE);
            return SDCARD_ERROR_HAL;
        }
        status = SDCard_WaitReady(SDCARD_DEFAULT_TIMEOUT_MS);
        if (status != SDCARD_OK)
        {
            return status;
        }
        block += transfer_blocks;
        block_count -= transfer_blocks;
        data += transfer_blocks * SDCARD_BLOCK_SIZE;
    }
    return SDCARD_OK;
}

/**
  * @brief  擦除一个连续的逻辑块范围
  * @param  first_block 首个逻辑块号
  * @param  last_block 最后一个逻辑块号，包含该块
  * @retval SDCard_Status_t 擦除结果
  */
SDCard_Status_t SDCard_EraseBlocks(uint32_t first_block, uint32_t last_block)
{
    SDCard_Status_t status;

    if (SDCard_Initialized == 0U)
    {
        return SDCARD_ERROR_NOT_INITIALIZED;
    }
    if (last_block < first_block)
    {
        return SDCARD_ERROR_ARGUMENT;
    }
    status = SDCard_CheckRange(first_block, last_block - first_block + 1U);
    if (status != SDCARD_OK)
    {
        return status;
    }
    if (HAL_SD_Erase(&SDCARD_HANDLE, first_block, last_block) != HAL_OK)
    {
        SDCard_LastHalError = HAL_SD_GetError(&SDCARD_HANDLE);
        return SDCARD_ERROR_HAL;
    }
    return SDCard_WaitReady(SDCARD_DEFAULT_TIMEOUT_MS);
}

/**
  * @brief  等待 SD 卡完成内部读写或擦除操作
  * @param  无
  * @retval SDCard_Status_t 同步结果
  */
SDCard_Status_t SDCard_Sync(void)
{
    if (SDCard_Initialized == 0U)
    {
        return SDCARD_ERROR_NOT_INITIALIZED;
    }
    return SDCard_WaitReady(SDCARD_DEFAULT_TIMEOUT_MS);
}

/* ==================== 文件 API 函数 ==================== */

/**
  * @brief  初始化 SD 卡并挂载 FAT 文件系统
  * @param  无
  * @retval FRESULT FatFs 执行结果
  */
FRESULT SDCard_Mount(void)
{
    FRESULT result;

    if ((SDCard_Mounted != 0U) && (SDCard_IsReady() != 0U))
    {
        return FR_OK;
    }
    SDCard_Mounted = 0U;
    if (SDCard_Init() != SDCARD_OK)
    {
        return FR_NOT_READY;
    }
    result = f_mount(&SDCard_FileSystem, "0:", 1U);
    if (result == FR_OK)
    {
        SDCard_Mounted = 1U;
    }
    return result;
}

/**
  * @brief  卸载 FAT 文件系统
  * @param  无
  * @retval FRESULT FatFs 执行结果
  */
FRESULT SDCard_Unmount(void)
{
    FRESULT result = f_mount(NULL, "0:", 0U);

    if (result == FR_OK)
    {
        SDCard_Mounted = 0U;
    }
    return result;
}

/**
  * @brief  查询 FAT 文件系统是否已经挂载
  * @param  无
  * @retval 1 已挂载，0 未挂载
  */
uint8_t SDCard_IsMounted(void)
{
    return SDCard_Mounted;
}

/**
  * @brief  创建或覆盖文件并写入数据
  * @param  path 文件路径，当前配置使用 8.3 短文件名
  * @param  data 待写入数据
  * @param  size 写入字节数
  * @param  bytes_written 实际写入字节数，可为 NULL
  * @retval FRESULT FatFs 执行结果
  */
FRESULT SDCard_WriteFile(const char *path, const void *data, uint32_t size,
                         uint32_t *bytes_written)
{
    FRESULT result;
    FRESULT close_result;
    UINT count = 0U;

    if (bytes_written != NULL)
    {
        *bytes_written = 0U;
    }
    if ((path == NULL) || ((data == NULL) && (size != 0U)))
    {
        return FR_INVALID_PARAMETER;
    }
    result = SDCard_Mount();
    if (result != FR_OK)
    {
        return result;
    }
    result = f_open(&SDCard_File, path, FA_WRITE | FA_CREATE_ALWAYS);
    if (result != FR_OK)
    {
        return result;
    }

    result = f_write(&SDCard_File, data, (UINT)size, &count);
    if ((result == FR_OK) && (count != (UINT)size))
    {
        result = FR_DISK_ERR;
    }
    if (result == FR_OK)
    {
        result = f_sync(&SDCard_File);
    }
    close_result = f_close(&SDCard_File);
    if (result == FR_OK)
    {
        result = close_result;
    }
    if (bytes_written != NULL)
    {
        *bytes_written = (uint32_t)count;
    }
    return result;
}

/**
  * @brief  从文件开头读取数据
  * @param  path 文件路径，当前配置使用 8.3 短文件名
  * @param  data 接收缓冲区
  * @param  size 最多读取字节数
  * @param  bytes_read 实际读取字节数，可为 NULL
  * @retval FRESULT FatFs 执行结果
  */
FRESULT SDCard_ReadFile(const char *path, void *data, uint32_t size,
                        uint32_t *bytes_read)
{
    FRESULT result;
    FRESULT close_result;
    UINT count = 0U;

    if (bytes_read != NULL)
    {
        *bytes_read = 0U;
    }
    if ((path == NULL) || ((data == NULL) && (size != 0U)))
    {
        return FR_INVALID_PARAMETER;
    }
    result = SDCard_Mount();
    if (result != FR_OK)
    {
        return result;
    }
    result = f_open(&SDCard_File, path, FA_READ);
    if (result != FR_OK)
    {
        return result;
    }

    result = f_read(&SDCard_File, data, (UINT)size, &count);
    close_result = f_close(&SDCard_File);
    if (result == FR_OK)
    {
        result = close_result;
    }
    if (bytes_read != NULL)
    {
        *bytes_read = (uint32_t)count;
    }
    return result;
}

/**
  * @brief  删除指定文件
  * @param  path 文件路径
  * @retval FRESULT FatFs 执行结果
  */
FRESULT SDCard_RemoveFile(const char *path)
{
    FRESULT result;

    if (path == NULL)
    {
        return FR_INVALID_PARAMETER;
    }
    result = SDCard_Mount();
    return (result == FR_OK) ? f_unlink(path) : result;
}

/**
  * @brief  获取 FAT 卷总容量和剩余容量
  * @param  total_kb 总容量输出地址，单位 KiB
  * @param  free_kb 剩余容量输出地址，单位 KiB
  * @retval FRESULT FatFs 执行结果
  */
FRESULT SDCard_GetSpace(uint32_t *total_kb, uint32_t *free_kb)
{
    FATFS *file_system;
    DWORD free_clusters;
    FRESULT result;

    if ((total_kb == NULL) || (free_kb == NULL))
    {
        return FR_INVALID_PARAMETER;
    }
    result = SDCard_Mount();
    if (result != FR_OK)
    {
        return result;
    }
    result = f_getfree("0:", &free_clusters, &file_system);
    if (result == FR_OK)
    {
        *free_kb = (uint32_t)(free_clusters * file_system->csize / 2U);
        *total_kb = (uint32_t)((file_system->n_fatent - 2U) *
                               file_system->csize / 2U);
    }
    return result;
}

/**
  * @brief  查询 SD 卡是否处于可传输状态
  * @param  无
  * @retval 1 SD 卡就绪，0 SD 卡未就绪
  */
uint8_t SDCard_IsReady(void)
{
    return ((SDCard_Initialized != 0U) &&
            (HAL_SD_GetCardState(&SDCARD_HANDLE) == HAL_SD_CARD_TRANSFER)) ? 1U : 0U;
}

/**
  * @brief  获取最近一次 STM32 HAL SD 错误位
  * @param  无
  * @retval uint32_t HAL SD 错误位
  */
uint32_t SDCard_GetHalError(void)
{
    return SDCard_LastHalError;
}

/**
  * @brief  将驱动状态转换为便于串口输出的文字
  * @param  status SD 卡驱动状态
  * @retval const char * 状态文字
  */
const char *SDCard_GetStatusText(SDCard_Status_t status)
{
    static const char *status_text[] =
    {
        "OK",
        "invalid argument",
        "not initialized",
        "not ready",
        "unsupported block size",
        "block out of range",
        "HAL error",
        "timeout"
    };

    if ((uint32_t)status >= (sizeof(status_text) / sizeof(status_text[0])))
    {
        return "unknown";
    }
    return status_text[(uint32_t)status];
}

/**
  * @brief  获取最近一次初始化停止的阶段
  * @param  无
  * @retval SDCard_InitStage_t 初始化阶段
  */
SDCard_InitStage_t SDCard_GetInitStage(void)
{
    return SDCard_InitStage;
}

/**
  * @brief  将初始化阶段转换为便于串口输出的文字
  * @param  stage 初始化阶段
  * @retval const char * 阶段文字
  */
const char *SDCard_GetInitStageText(SDCard_InitStage_t stage)
{
    static const char *stage_text[] =
    {
        "idle",
        "HAL_SD_Init",
        "4-bit bus",
        "card info",
        "wait ready",
        "ready"
    };

    if ((uint32_t)stage >= (sizeof(stage_text) / sizeof(stage_text[0])))
    {
        return "unknown";
    }
    return stage_text[(uint32_t)stage];
}

/* ==================== SD 卡模块测试函数 ==================== */

/**
  * @brief  初始化并执行非破坏性的 SD 卡读取测试
  * @param  无
  * @retval SDCard_Status_t 测试结果
  */
SDCard_Status_t SDCard_Test_Init(void)
{
    SDCard_Info_t info;
    SDCard_Status_t status;
    uint32_t checksum = 0U;
    uint32_t index;

    printf("\r\n[SDCard] raw block test\r\n");
    status = SDCard_Init();
    if (status != SDCARD_OK)
    {
        printf("[SDCard] init failed: %d\r\n", (int)status);
        return status;
    }
    status = SDCard_GetInfo(&info);
    if (status != SDCARD_OK)
    {
        printf("[SDCard] get info failed: %d\r\n", (int)status);
        return status;
    }
    status = SDCard_ReadBlocks(0U, SDCard_TestBuffer, 1U);
    if (status != SDCARD_OK)
    {
        printf("[SDCard] block 0 read failed: %d\r\n", (int)status);
        return status;
    }
    for (index = 0U; index < SDCARD_BLOCK_SIZE; index++)
    {
        checksum += SDCard_TestBuffer[index];
    }
    printf("[SDCard] blocks=%lu, capacity=%lu MB, block0 checksum=%lu\r\n",
           (unsigned long)info.block_count,
           (unsigned long)(info.capacity_bytes / (1024ULL * 1024ULL)),
           (unsigned long)checksum);
    return SDCARD_OK;
}

/**
  * @brief  SD 卡模块测试轮询入口
  * @param  无
  * @retval 无
  */
void SDCard_Test_Poll(void)
{
    /* 当前测试在初始化阶段完成，无需周期处理。 */
}

#endif /* DEVICE_SDCARD */

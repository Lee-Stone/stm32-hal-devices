/* FatFs physical drive 0 adapter.
 *
 * This file translates FatFs disk operations into the independent SDCard
 * block-device API. It contains no SDIO or STM32 HAL access.
 */
#include "config.h"

#if DEVICE_SDCARD

#include "diskio.h"
#include "SDCard.h"

#define FATFS_DISK_DRIVE  0U

static DSTATUS FatFsDisk_Status = STA_NOINIT;

DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv != FATFS_DISK_DRIVE)
    {
        return STA_NOINIT;
    }

    FatFsDisk_Status = (SDCard_Init() == SDCARD_OK) ? 0U : STA_NOINIT;
    return FatFsDisk_Status;
}

DSTATUS disk_status(BYTE pdrv)
{
    if (pdrv != FATFS_DISK_DRIVE)
    {
        return STA_NOINIT;
    }
    if ((FatFsDisk_Status == 0U) && (SDCard_IsReady() == 0U))
    {
        FatFsDisk_Status = STA_NOINIT;
    }
    return FatFsDisk_Status;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count)
{
    SDCard_Status_t status;

    if ((pdrv != FATFS_DISK_DRIVE) || (buff == NULL) || (count == 0U))
    {
        return RES_PARERR;
    }
    if ((disk_status(pdrv) & STA_NOINIT) != 0U)
    {
        return RES_NOTRDY;
    }
    status = SDCard_ReadBlocks((uint32_t)sector, buff, (uint32_t)count);
    if ((status == SDCARD_ERROR_ARGUMENT) || (status == SDCARD_ERROR_RANGE))
    {
        return RES_PARERR;
    }
    if ((status == SDCARD_ERROR_NOT_INITIALIZED) ||
        (status == SDCARD_ERROR_NOT_READY))
    {
        FatFsDisk_Status = STA_NOINIT;
        return RES_NOTRDY;
    }
    return (status == SDCARD_OK) ? RES_OK : RES_ERROR;
}

#if _USE_WRITE
DRESULT disk_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count)
{
    SDCard_Status_t status;

    if ((pdrv != FATFS_DISK_DRIVE) || (buff == NULL) || (count == 0U))
    {
        return RES_PARERR;
    }
    if ((disk_status(pdrv) & STA_NOINIT) != 0U)
    {
        return RES_NOTRDY;
    }
    status = SDCard_WriteBlocks((uint32_t)sector, buff, (uint32_t)count);
    if ((status == SDCARD_ERROR_ARGUMENT) || (status == SDCARD_ERROR_RANGE))
    {
        return RES_PARERR;
    }
    if ((status == SDCARD_ERROR_NOT_INITIALIZED) ||
        (status == SDCARD_ERROR_NOT_READY))
    {
        FatFsDisk_Status = STA_NOINIT;
        return RES_NOTRDY;
    }
    return (status == SDCARD_OK) ? RES_OK : RES_ERROR;
}
#endif

#if _USE_IOCTL
DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    SDCard_Info_t info;

    if (pdrv != FATFS_DISK_DRIVE)
    {
        return RES_PARERR;
    }
    if ((cmd != CTRL_SYNC) && (buff == NULL))
    {
        return RES_PARERR;
    }
    if ((disk_status(pdrv) & STA_NOINIT) != 0U)
    {
        return RES_NOTRDY;
    }

    switch (cmd)
    {
        case CTRL_SYNC:
            return (SDCard_Sync() == SDCARD_OK) ? RES_OK : RES_ERROR;

        case GET_SECTOR_COUNT:
            if (SDCard_GetInfo(&info) != SDCARD_OK)
            {
                return RES_ERROR;
            }
            *(DWORD *)buff = (DWORD)info.block_count;
            return RES_OK;

        case GET_SECTOR_SIZE:
            *(WORD *)buff = (WORD)SDCARD_BLOCK_SIZE;
            return RES_OK;

        case GET_BLOCK_SIZE:
            *(DWORD *)buff = 1U;
            return RES_OK;

        default:
            return RES_PARERR;
    }
}
#endif

#endif /* DEVICE_SDCARD */

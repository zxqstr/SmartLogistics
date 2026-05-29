#include "ff.h"
#include "diskio.h"
#include "SDCard.h"

DSTATUS disk_initialize(BYTE pdrv)
{
	if (pdrv) return STA_NOINIT;
	if (SD_Init() == 0)
		return 0;
	else
		return STA_NOINIT;
}

DSTATUS disk_status(BYTE pdrv)
{
	if (pdrv) return STA_NOINIT;
	return 0;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
	if (pdrv || count == 0) return RES_PARERR;

	while (count--)
	{
		if (SD_ReadBlock(sector++, buff))
			return RES_ERROR;
		buff += 512;
	}
	return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
	if (pdrv || count == 0) return RES_PARERR;

	while (count--)
	{
		if (SD_WriteBlock(sector++, buff))
			return RES_ERROR;
		buff += 512;
	}
	return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
	if (pdrv) return RES_PARERR;

	switch (cmd)
	{
	case CTRL_SYNC:
		return RES_OK;

	case GET_SECTOR_COUNT:
		*(LBA_t *)buff = SD_GetSectorCount();
		return RES_OK;

	case GET_SECTOR_SIZE:
		*(WORD *)buff = 512;
		return RES_OK;

	case GET_BLOCK_SIZE:
		*(DWORD *)buff = 1;
		return RES_OK;

	default:
		return RES_PARERR;
	}
}

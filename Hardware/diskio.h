#ifndef _DISKIO_H
#define _DISKIO_H

#include "stm32f10x.h"
#include "ff.h"

/* FATFS需要的额外类型 */
typedef uint32_t LBA_t;

/* 磁盘状态 */
typedef BYTE DSTATUS;
#define STA_NOINIT  0x01
#define STA_NODISK  0x02
#define STA_PROTECT 0x04

/* 磁盘操作结果 */
typedef enum {
	RES_OK = 0,
	RES_ERROR,
	RES_WRPRT,
	RES_NOTRDY,
	RES_PARERR
} DRESULT;

/* disk_ioctl 命令 */
#define CTRL_SYNC          0
#define GET_SECTOR_COUNT   1
#define GET_SECTOR_SIZE    2
#define GET_BLOCK_SIZE     3
#define CTRL_TRIM          4

/* 外部函数 */
DSTATUS disk_initialize(BYTE pdrv);
DSTATUS disk_status(BYTE pdrv);
DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count);
DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count);
DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff);

#endif

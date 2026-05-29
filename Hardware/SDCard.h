#ifndef __SDCARD_H
#define __SDCARD_H

#include "stm32f10x.h"

/* SD卡类型 */
#define SD_TYPE_ERR  0
#define SD_TYPE_V1   1
#define SD_TYPE_V2   2
#define SD_TYPE_V2HC 3

uint8_t SD_Init(void);
uint8_t SD_ReadBlock(uint32_t sector, uint8_t *buf);
uint8_t SD_WriteBlock(uint32_t sector, const uint8_t *buf);
uint8_t SD_GetType(void);
uint32_t SD_GetSectorCount(void);

#endif

#include "SDCard.h"
#include "MySPI.h"
#include "Delay.h"

static uint8_t  sd_type = 0;
static uint32_t sd_sectors = 0;

uint8_t SD_GetType(void)   { return sd_type; }
uint32_t SD_GetSectorCount(void) { return sd_sectors; }

/* 发送SD命令: 先等卡就绪(MISO高), 再发6字节命令 */
static uint8_t SD_SendCmd(uint8_t cmd, uint32_t arg, uint8_t crc)
{
	uint8_t r1;
	uint16_t i;

	/* 等待卡就绪: MISO为高, 最多等200次 */
	for (i = 0; i < 200; i++)
	{
		if (MySPI_SwapByte(0xFF) == 0xFF) break;
	}
	if (i == 200) return 0xFF;  /* 超时 */

	/* 发送命令6字节 */
	MySPI_SwapByte(cmd | 0x40);
	MySPI_SwapByte(arg >> 24);
	MySPI_SwapByte(arg >> 16);
	MySPI_SwapByte(arg >> 8);
	MySPI_SwapByte(arg);
	MySPI_SwapByte(crc);

	/* 等待R1响应(N*8个时钟, 最多8字节) */
	for (i = 0; i < 8; i++)
	{
		r1 = MySPI_SwapByte(0xFF);
		if ((r1 & 0x80) == 0) break;  /* 最高位=0表示有效响应 */
	}
	return r1;
}

uint8_t SD_Init(void)
{
	uint8_t  r1;
	uint16_t t;

	MySPI_Init();

	/* 1. 上电初始化: CS低, 发送80个时钟(HW-125缓冲器CS低才通) */
	MySPI_Start();  /* CS低, 缓冲器通 */
	for (t = 0; t < 10; t++)
		MySPI_SwapByte(0xFF);
	MySPI_Stop();   /* CS高 */
	Delay_ms(10);

	/* 2. CMD0: 进入SPI模式 */
	MySPI_Start();
	for (t = 0; t < 10; t++)
	{
		Delay_ms(1);  /* 每次尝试间隔1ms */
		r1 = SD_SendCmd(0, 0, 0x95);
		if (r1 == 0x01) break;
		/* 失败则重置: CS高再低 */
		MySPI_Stop();
		Delay_ms(1);
		MySPI_Start();
	}
	if (r1 != 0x01) { MySPI_Stop(); sd_type = SD_TYPE_ERR; return 1; }
	MySPI_Stop();

	/* 3. CMD8: 区分V1/V2卡 */
	MySPI_Start();
	r1 = SD_SendCmd(8, 0x1AA, 0x87);
	MySPI_Stop();

	if (r1 == 0x01)
	{
		/* V2卡: 读R7响应的4字节 */
		/* (R7已有, 不用再读) */
		/* 读掉4字节R7 */
		MySPI_Start();
		MySPI_SwapByte(0xFF);
		MySPI_SwapByte(0xFF);
		MySPI_SwapByte(0xFF);
		MySPI_SwapByte(0xFF);
		MySPI_Stop();

		/* ACMD41 */
		MySPI_Start();
		for (t = 0; t < 1000; t++)
		{
			SD_SendCmd(55, 0, 0x65);       /* CMD55 = APP_CMD前缀 */
			r1 = SD_SendCmd(41, 0x40000000, 0x77); /* ACMD41 HCS=1 */
			if (r1 == 0x00) break;
			Delay_ms(1);
		}
		MySPI_Stop();

		if (r1 != 0x00) { sd_type = SD_TYPE_ERR; return 2; }

		/* 读OCR判断SDHC */
		MySPI_Start();
		SD_SendCmd(58, 0, 0xFD);
		uint8_t ocr = MySPI_SwapByte(0xFF);
		MySPI_SwapByte(0xFF);
		MySPI_SwapByte(0xFF);
		MySPI_SwapByte(0xFF);
		MySPI_Stop();

		if (ocr & 0x40)
			sd_type = SD_TYPE_V2HC;
		else
			sd_type = SD_TYPE_V2;
	}
	else
	{
		/* V1/MMC卡 */
		MySPI_Start();
		for (t = 0; t < 1000; t++)
		{
			SD_SendCmd(55, 0, 0x65);
			r1 = SD_SendCmd(41, 0, 0x77);
			if (r1 == 0x00) break;
			Delay_ms(1);
		}
		MySPI_Stop();

		if (r1 != 0x00) { sd_type = SD_TYPE_ERR; return 3; }
		sd_type = SD_TYPE_V1;
	}

	/* 读CSD */
	MySPI_Start();
	r1 = SD_SendCmd(9, 0, 0xAF);
	if (r1 != 0x00) { MySPI_Stop(); return 4; }

	/* 等数据令牌0xFE */
	for (t = 0; t < 50000; t++)
	{
		r1 = MySPI_SwapByte(0xFF);
		if (r1 == 0xFE) break;
	}
	if (r1 != 0xFE) { MySPI_Stop(); return 5; }

	/* 读16字节CSD */
	uint8_t csd[16];
	for (t = 0; t < 16; t++)
		csd[t] = MySPI_SwapByte(0xFF);
	MySPI_SwapByte(0xFF);  /* CRC */
	MySPI_SwapByte(0xFF);
	MySPI_Stop();

	/* 计算容量 */
	if (sd_type == SD_TYPE_V2HC)
	{
		sd_sectors = ((uint32_t)(csd[7] & 0x3F) << 16) |
		             ((uint32_t)csd[8] << 8) | csd[9];
		sd_sectors = (sd_sectors + 1) * 1024;
	}
	else if (sd_type == SD_TYPE_V2)
	{
		uint32_t csz = ((uint32_t)(csd[6] & 0x03) << 10) |
		               ((uint32_t)csd[7] << 2) | (csd[8] >> 6);
		sd_sectors = (csz + 1) * 1024;
	}
	else
	{
		uint32_t csz = ((uint32_t)(csd[6] & 0x03) << 10) |
		               ((uint32_t)csd[7] << 2) | (csd[8] >> 6);
		uint8_t  cmult = (csd[9] & 0x03) << 1 | (csd[10] >> 7);
		uint8_t  rblen = csd[5] & 0x0F;
		sd_sectors = (csz + 1) * (1UL << (cmult + 2));
		sd_sectors >>= (9 - rblen);
	}

	/* CMD16: 设块大小512 */
	MySPI_Start();
	SD_SendCmd(16, 512, 0x15);
	MySPI_Stop();

	return 0;
}

uint8_t SD_ReadBlock(uint32_t sector, uint8_t *buf)
{
	uint8_t r1;
	uint16_t r, t;

	if (sd_type != SD_TYPE_V2HC)
		sector <<= 9;

	MySPI_Start();
	if (SD_SendCmd(17, sector, 0x77)) { MySPI_Stop(); return 1; }

	for (t = 0; t < 50000; t++)
	{
		r1 = MySPI_SwapByte(0xFF);
		if (r1 == 0xFE) break;
	}
	if (r1 != 0xFE) { MySPI_Stop(); return 2; }

	for (r = 0; r < 512; r++)
		buf[r] = MySPI_SwapByte(0xFF);

	MySPI_SwapByte(0xFF);  /* CRC */
	MySPI_SwapByte(0xFF);
	MySPI_Stop();
	return 0;
}

uint8_t SD_WriteBlock(uint32_t sector, const uint8_t *buf)
{
	uint8_t r1;
	uint16_t r, t;

	if (sd_type != SD_TYPE_V2HC)
		sector <<= 9;

	MySPI_Start();
	if (SD_SendCmd(24, sector, 0x77)) { MySPI_Stop(); return 1; }

	MySPI_SwapByte(0xFE);
	for (r = 0; r < 512; r++)
		MySPI_SwapByte(buf[r]);
	MySPI_SwapByte(0xFF);  /* CRC */
	MySPI_SwapByte(0xFF);

	r1 = MySPI_SwapByte(0xFF);
	if ((r1 & 0x1F) != 0x05) { MySPI_Stop(); return 2; }

	for (t = 0; t < 50000; t++)
	{
		if (MySPI_SwapByte(0xFF) != 0x00) break;
	}

	MySPI_Stop();
	return (t == 50000) ? 3 : 0;
}

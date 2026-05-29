#include "DataLog.h"
#include "ff.h"
#include <stdio.h>

static FATFS fs;
static FIL  logfile;
static uint8_t mounted = 0;

uint8_t DataLog_Init(void)
{
	FRESULT res;

	res = f_mount(&fs, "0:", 1);
	if (res != FR_OK) return 1;
	mounted = 1;

	/* 打开文件(不存在则创建), 追加写入CSV头 */
	res = f_open(&logfile, "0:data.csv", FA_OPEN_APPEND | FA_WRITE);
	if (res != FR_OK)
	{
		/* 文件不存在, 新建并写表头 */
		res = f_open(&logfile, "0:data.csv", FA_CREATE_ALWAYS | FA_WRITE);
		if (res != FR_OK) return 2;

		f_printf(&logfile, "timestamp,temp,humi,lat,lon,speed,gps_valid\n");
	}
	f_close(&logfile);

	return 0;
}

uint8_t DataLog_Write(float temp, float humi,
                      float lat, float lon, float speed,
                      uint8_t gps_valid)
{
	FRESULT res;

	if (!mounted) return 1;

	res = f_open(&logfile, "0:data.csv", FA_OPEN_APPEND | FA_WRITE);
	if (res != FR_OK) return 2;

	f_printf(&logfile, "2026-05-30 00:00,");
	f_printf(&logfile, "%.1f,%.1f,", temp, humi);
	f_printf(&logfile, "%.5f,%.5f,", lat, lon);
	f_printf(&logfile, "%.1f,%d\n", speed, gps_valid);

	f_close(&logfile);
	return 0;
}

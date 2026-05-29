#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "DHT11.h"
#include "Key.h"
#include "Buzzer.h"
#include "GPS.h"
#include "SDCard.h"
#include "ff.h"

typedef enum {
	PAGE_MAIN = 0,
	PAGE_SD,
	PAGE_SYSINFO,
	PAGE_MAX
} Page_t;

static Page_t page = PAGE_MAIN;
static float temp = 0, humi = 0;

static void ShowPageMain(void)
{
	OLED_ShowString(1, 1, "--Main Page-----");

	OLED_ShowString(2, 1, "T:");
	OLED_ShowNum(2, 3, (uint32_t)temp, 2);
	OLED_ShowString(2, 5, ".");
	OLED_ShowNum(2, 6, (int32_t)(temp * 10) % 10, 1);
	OLED_ShowString(2, 7, "C ");

	OLED_ShowString(2, 10, "H:");
	OLED_ShowNum(2, 12, (uint32_t)humi, 2);
	OLED_ShowString(2, 14, ".");
	OLED_ShowNum(2, 15, (int32_t)(humi * 10) % 10, 1);
	OLED_ShowChar(2, 16, '%');

	OLED_ShowString(3, 1, "GPS: No Fix    ");
	OLED_ShowString(4, 1, "K1:Next K2:SD ");
}

static void ShowPageSD(void)
{
	FATFS fs;
	FIL   file;
	FRESULT res;
	char buf[32];
	UINT br;

	OLED_ShowString(1, 1, "--SD FATFS Test-");

	/* 挂载 */
	res = f_mount(&fs, "0:", 1);
	if (res != FR_OK)
	{
		OLED_ShowString(2, 1, "Mount FAIL:");
		OLED_ShowNum(2, 12, res, 1);
		return;
	}
	OLED_ShowString(2, 1, "Mount:OK       ");

	/* 写测试文件 */
	res = f_open(&file, "0:test.txt", FA_CREATE_ALWAYS | FA_WRITE);
	if (res != FR_OK)
	{
		OLED_ShowString(3, 1, "Open FAIL:");
		OLED_ShowNum(3, 11, res, 1);
		return;
	}

	f_printf(&file, "STM32 SD Card Test OK!\n");
	f_printf(&file, "T=%.1fC H=%.1f%%\n", temp, humi);
	f_close(&file);

	/* 读回验证 */
	res = f_open(&file, "0:test.txt", FA_READ);
	if (res != FR_OK)
	{
		OLED_ShowString(3, 1, "Read FAIL:");
		OLED_ShowNum(3, 11, res, 1);
		return;
	}

	f_read(&file, buf, 31, &br);
	buf[br] = '\0';
	f_close(&file);

	OLED_ShowString(3, 1, "Write+Read: OK ");
	OLED_ShowString(4, 1, buf);  /* 第4行显示前16字节 */

	f_mount(0, "0:", 0);  /* 卸载 */
}

static void ShowPageSysInfo(void)
{
	OLED_ShowString(1, 1, "--System Info---");
	OLED_ShowString(2, 1, "FW: v0.3 FATFS ");
	OLED_ShowString(3, 1, "DHT11+SD+FATFS ");
	OLED_ShowString(4, 1, "K1:Back        ");
}

int main(void)
{
	uint8_t key;
	uint16_t dht_timer = 0;

	OLED_Init();
	OLED_Clear();
	OLED_ShowString(1, 1, "Booting...");

	Key_Init();
	Buzzer_Init();
	DHT11_Init();

	OLED_ShowString(2, 1, "SD Init...");
	uint8_t sd_err = SD_Init();
	OLED_ShowString(3, 1, "SD:");
	if (sd_err == 0)
		OLED_ShowString(3, 4, "OK  ");
	else
	{
		OLED_ShowString(3, 4, "ERR ");
		OLED_ShowNum(3, 8, sd_err, 1);
	}

	Delay_ms(1500);
	OLED_Clear();

	while (1)
	{
		key = Key_Scan();

		if (key == KEY_K1)
		{
			page = (page + 1) % PAGE_MAX;
			OLED_Clear();
		}

		switch (page)
		{
		case PAGE_MAIN:
			ShowPageMain();
			if (key == KEY_K2)
				Buzzer_Alarm(ALARM_TEMP_HIGH);
			if (key == KEY_K3)
				Buzzer_Stop();
			break;

		case PAGE_SD:
			{	static uint8_t tested = 0;
				if (!tested) { ShowPageSD(); tested = 1; }
			}
			break;

		case PAGE_SYSINFO:
			ShowPageSysInfo();
			break;
		}

		dht_timer++;
		if (dht_timer >= 100)
		{
			dht_timer = 0;
			if (DHT11_ReadData(&temp, &humi) != 0)
				temp = 0, humi = 0;
		}

		Buzzer_Tick();
		Delay_ms(20);
	}
}

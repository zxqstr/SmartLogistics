#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "DHT11.h"
#include "Key.h"
#include "Buzzer.h"
#include "GPS.h"
#include "SDCard.h"

typedef enum {
	PAGE_MAIN = 0,
	PAGE_SD,
	PAGE_SYSINFO,
	PAGE_MAX
} Page_t;

static Page_t page = PAGE_MAIN;
static float temp = 0, humi = 0;
static float temp_max = 40.0f, humi_max = 80.0f;
static float temp_min = 0.0f,  humi_min = 20.0f;

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

	OLED_ShowString(3, 1, "GPS:");
	if (gps.valid) {
		OLED_ShowNum(3, 5, (uint32_t)gps.speed_kn, 2);
		OLED_ShowString(3, 7, "kn");
	} else {
		OLED_ShowString(3, 5, "No Fix");
	}

	OLED_ShowString(4, 1, "K1:Next K2:SD");
}

static void ShowPageSD(void)
{
	OLED_ShowString(1, 1, "--SD Card Test--");

	uint32_t sec = SD_GetSectorCount();
	uint8_t  typ = SD_GetType();

	if (typ == SD_TYPE_ERR)
	{
		OLED_ShowString(2, 1, "SD: Not Found!");
		OLED_ShowString(3, 1, "Check wiring");
	}
	else
	{
		OLED_ShowString(2, 1, "SD: Found!");
		OLED_ShowString(2, 11, "T:");
		OLED_ShowNum(2, 13, typ, 1);

		OLED_ShowString(3, 1, "Size:");
		OLED_ShowNum(3, 7, sec / 2048, 6);  /* MB */
		OLED_ShowString(3, 13, "MB");
	}
	OLED_ShowString(4, 1, "K1:Back       ");
}

static void ShowPageSysInfo(void)
{
	OLED_ShowString(1, 1, "--System Info---");
	OLED_ShowString(2, 1, "FW: v0.2 SD+  ");
	OLED_ShowString(3, 1, "DHT11 + SD SPI ");
	OLED_ShowString(4, 1, "K1:Back        ");
}

int main(void)
{
	uint8_t key;
	uint16_t dht_timer = 0;

	OLED_Init();
	OLED_Clear();

	Key_Init();
	Buzzer_Init();
	DHT11_Init();

	/* 显示SD初始化状态 */
	OLED_ShowString(1, 1, "Initializing...");
	OLED_ShowString(2, 1, "SD Card Init...");

	uint8_t sd_err = SD_Init();

	OLED_ShowString(3, 1, "SD Ret:");
	OLED_ShowNum(3, 8, sd_err, 1);
	if (sd_err == 0)
		OLED_ShowString(4, 1, "SD: OK!       ");
	else
		OLED_ShowString(4, 1, "SD: FAIL!     ");

	Delay_ms(1500);
	OLED_Clear();

	while (1)
	{
		key = Key_Scan();

		/* K1: 切换界面 */
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
			ShowPageSD();
			break;

		case PAGE_SYSINFO:
			ShowPageSysInfo();
			break;
		}

		/* DHT11: 每2秒 */
		dht_timer++;
		if (dht_timer >= 100)
		{
			dht_timer = 0;
			if (DHT11_ReadData(&temp, &humi) != 0)
			{
				temp = 0; humi = 0;
			}
		}

		Buzzer_Tick();
		Delay_ms(20);
	}
}

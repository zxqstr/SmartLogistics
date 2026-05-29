#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "DHT11.h"
#include "Key.h"
#include "Buzzer.h"
#include "GPS.h"

typedef enum {
	PAGE_MAIN = 0,
	PAGE_THRESHOLD,
	PAGE_SYSINFO,
	PAGE_MAX
} Page_t;

static Page_t page = PAGE_MAIN;
static float temp_max = 40.0f, temp_min = 0.0f;
static float humi_max = 80.0f, humi_min = 20.0f;
static float temp = 0, humi = 0;

static void ShowPageMain(void)
{
	OLED_ShowString(1, 1, "--Main Page-----");

	/* 第2行: 温湿度 */
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

	/* 第3行: GPS状态 */
	{
		static uint8_t has_data = 0;
		static uint8_t dot = 0;

		GPS_Poll();  /* 轮询接收 */

		if (gps.data_ready)
		{
			has_data = 1;
			dot = !dot;
			GPS_Parse();
		}

		if (gps.valid)
		{
			OLED_ShowString(3, 1, "GPS:OK  Spd:");
			OLED_ShowNum(3, 13, (uint32_t)gps.speed_kn, 2);
			OLED_ShowString(3, 15, "kn");
		}
		else if (has_data)
		{
			if (dot)
				OLED_ShowString(3, 1, "GPS:Searching.  ");
			else
				OLED_ShowString(3, 1, "GPS:Searching.. ");
		}
		else
		{
			OLED_ShowString(3, 1, "GPS:No RX:");
			OLED_ShowNum(3, 11, gps.rx_count, 5);
		}
	}

	/* 第4行: 报警状态 */
	OLED_ShowString(4, 1, "Alarm:");
	if (temp > temp_max)      OLED_ShowString(4, 7, "T HIGH");
	else if (temp < temp_min) OLED_ShowString(4, 7, "T LOW ");
	else if (humi > humi_max) OLED_ShowString(4, 7, "H HIGH");
	else if (humi < humi_min) OLED_ShowString(4, 7, "H LOW ");
	else                      OLED_ShowString(4, 7, "Normal");
}

static void ShowPageThreshold(void)
{
	OLED_ShowString(1, 1, "--Thresholds----");
	OLED_ShowString(2, 1, "Tmax:");
	OLED_ShowNum(2, 6, (uint32_t)temp_max, 2);
	OLED_ShowString(2, 8, "C");

	OLED_ShowString(2, 11, "Tmin:");
	OLED_ShowNum(2, 16, (uint32_t)temp_min, 1);

	OLED_ShowString(3, 1, "Hmax:");
	OLED_ShowNum(3, 6, (uint32_t)humi_max, 2);
	OLED_ShowString(3, 8, "%");

	OLED_ShowString(3, 11, "Hmin:");
	OLED_ShowNum(3, 16, (uint32_t)humi_min, 1);

	OLED_ShowString(4, 1, "K1:Next K2/3:+/-");
}

static void ShowPageSysInfo(void)
{
	OLED_ShowString(1, 1, "--System Info---");
	OLED_ShowString(2, 1, "FW: v0.1       ");
	OLED_ShowString(3, 1, "Sensor: DHT11  ");
	OLED_ShowString(4, 1, "K1:Back        ");
}

int main(void)
{
	uint8_t key, dht_ok;
	uint16_t dht_timer = 0;

	OLED_Init();
	OLED_Clear();

	Key_Init();
	Buzzer_Init();
	DHT11_Init();
	GPS_Init();

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

		case PAGE_THRESHOLD:
			ShowPageThreshold();
			break;

		case PAGE_SYSINFO:
			ShowPageSysInfo();
			break;
		}

		/* 每2秒读一次DHT11 */
		dht_timer++;
		if (dht_timer >= 100)  /* 100 × 20ms = 2000ms */
		{
			dht_timer = 0;
			dht_ok = (DHT11_ReadData(&temp, &humi) == 0);
			if (!dht_ok)
			{
				temp = 0;
				humi = 0;
			}
		}

		Buzzer_Tick();
		Delay_ms(20);
	}
}

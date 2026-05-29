#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "DHT11.h"
#include "Key.h"
#include "Buzzer.h"
#include "GPS.h"
#include "SDCard.h"
#include "ff.h"
#include "ESP8266.h"

/* ThingsCloud */
#define WIFI_SSID       "Qis"
#define WIFI_PWD        "lsp66666"
#define MQTT_HOST       "sh-3-mqtt.iot-api.com"
#define MQTT_PORT       1883
#define MQTT_CLIENT_ID  "stm32_logistics_001"
#define MQTT_USER       "q0cjd5u4zbkamop1"
#define MQTT_PASS       "TIitoajQdg"
#define MQTT_PUB_TOPIC  "device/data"
#define MQTT_SUB_TOPIC  "device/cmd"

typedef enum {
	PAGE_MAIN = 0, PAGE_GPS, PAGE_SD, PAGE_WIFI, PAGE_SYSINFO, PAGE_MAX
} Page_t;

static Page_t page = PAGE_MAIN;
static float temp = 0, humi = 0;
static uint8_t wifi_ok = 0, mqtt_ok = 0;

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
	if (gps.valid) OLED_ShowString(3, 5, "OK ");
	else           OLED_ShowString(3, 5, "NoFix");

	OLED_ShowString(3, 10, "WiFi:");
	if (mqtt_ok)           OLED_ShowString(3, 15, "OK");
	else if (wifi_ok)      OLED_ShowString(3, 15, "WiF");
	else                   OLED_ShowString(3, 15, "---");

	OLED_ShowString(4, 1, "K1:Next       ");
}

static void ShowPageGPS(void)
{
	OLED_ShowString(1, 1, "--GPS Data------");
	GPS_Poll();
	if (gps.data_ready) GPS_Parse();
	if (gps.valid)
	{
		OLED_ShowString(2, 1, "L:");
		OLED_ShowNum(2, 3, (int32_t)gps.latitude % 100, 2);
		OLED_ShowString(2, 5, ".");
		OLED_ShowNum(2, 6, (int32_t)(gps.latitude * 100) % 100, 2);
		OLED_ShowString(2, 9, "E:");
		OLED_ShowNum(2, 11, (int32_t)gps.longitude % 1000, 3);
		OLED_ShowString(2, 14, ".");
		OLED_ShowNum(2, 15, (int32_t)(gps.longitude * 100) % 100, 1);
		uint16_t spd = (uint16_t)(gps.speed_kn * 1.852f);
		OLED_ShowString(3, 1, "Spd:");
		OLED_ShowNum(3, 5, spd, 3);
		OLED_ShowString(3, 8, "km/h");
		OLED_ShowString(4, 1, "RX:");
		OLED_ShowNum(4, 4, gps.rx_count / 100, 6);
	}
	else
	{
		OLED_ShowString(2, 1, "Waiting fix...");
		OLED_ShowString(3, 1, "RX:");
		OLED_ShowNum(3, 4, gps.rx_count, 6);
		OLED_ShowString(4, 1, "LED=1Hz=OK    ");
	}
}

static void ShowPageSD(void)
{
	FATFS fs; FIL file; FRESULT res;
	char buf[32]; UINT br;
	OLED_ShowString(1, 1, "--SD FATFS Test-");
	res = f_mount(&fs, "0:", 1);
	if (res != FR_OK) { OLED_ShowString(2, 1, "Mount FAIL"); return; }
	OLED_ShowString(2, 1, "Mount:OK       ");
	res = f_open(&file, "0:test.txt", FA_CREATE_ALWAYS | FA_WRITE);
	if (res != FR_OK) { OLED_ShowString(3, 1, "Open FAIL"); return; }
	f_printf(&file, "T=%.1f H=%.1f\n", temp, humi);
	f_close(&file);
	res = f_open(&file, "0:test.txt", FA_READ);
	f_read(&file, buf, 31, &br); buf[br] = '\0'; f_close(&file);
	OLED_ShowString(3, 1, "Write+Read: OK ");
	OLED_ShowString(4, 1, buf);
	f_mount(0, "0:", 0);
}

static void ShowPageWiFi(void)
{
	OLED_ShowString(1, 1, "--WiFi/MQTT-----");
	if (mqtt_ok)           OLED_ShowString(2, 1, "MQTT Connected!");
	else if (wifi_ok)      OLED_ShowString(2, 1, "WiFi OK,No MQTT");
	else                   OLED_ShowString(2, 1, "Not Connected  ");
	OLED_ShowString(3, 1, "Srv:");
	OLED_ShowString(3, 5, MQTT_HOST);
	OLED_ShowString(4, 1, "K2:Upload Test ");
}

static void ShowPageSysInfo(void)
{
	OLED_ShowString(1, 1, "--System Info---");
	OLED_ShowString(2, 1, "FW: v1.0 MQTT  ");
	OLED_ShowString(3, 1, "All modules OK ");
	OLED_ShowString(4, 1, "K1:Back        ");
}

int main(void)
{
	uint8_t key;
	uint16_t dht_timer = 0, upload_timer = 0;
	int j;

	/* 闪LED确认启动 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	GPIO_InitTypeDef led;
	led.GPIO_Mode = GPIO_Mode_Out_PP;
	led.GPIO_Pin = GPIO_Pin_13;
	led.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &led);
	for (j = 0; j < 5; j++)
	{
		GPIO_ResetBits(GPIOC, GPIO_Pin_13); Delay_ms(200);
		GPIO_SetBits(GPIOC, GPIO_Pin_13);   Delay_ms(200);
	}

	/* 基础初始化 */
	OLED_Init();
	OLED_Clear();
	OLED_ShowString(1, 1, "Init basic...");

	Key_Init();
	Buzzer_Init();
	DHT11_Init();
	OLED_ShowString(2, 1, "Basic OK");

	/* GPS */
	GPS_Init();
	OLED_ShowString(3, 1, "GPS OK");

	/* SD卡 */
	SD_Init();
	OLED_ShowString(4, 1, "SD OK");

	Delay_ms(1000);
	OLED_Clear();

	/* ESP8266: 最后初始化, 放在主循环中异步连接 */
	ESP8266_Init();
	/* WiFi + MQTT放到主循环首页中异步执行 */

	while (1)
	{
		key = Key_Scan();

		if (key == KEY_K1)
		{
			page = (page + 1) % PAGE_MAX;
			OLED_Clear();
		}

		/* ESP8266异步连接(一次性) */
		{
			static uint8_t esp_step = 0;
			if (esp_step == 0)
			{
				esp_step = 1; /* 延迟一帧 */
			}
			else if (esp_step == 1)
			{
				if (ESP_ConnectWiFi(WIFI_SSID, WIFI_PWD) == 0)
				{
					wifi_ok = 1;
					esp_step = 2;
				}
			}
			else if (esp_step == 2)
			{
				if (ESP_MQTT_Config(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS) == 0 &&
				    ESP_MQTT_Connect(MQTT_HOST, MQTT_PORT) == 0)
				{
					ESP_MQTT_Subscribe(MQTT_SUB_TOPIC);
					mqtt_ok = 1;
					esp_step = 3;
				}
			}
		}

		switch (page)
		{
		case PAGE_MAIN:
			ShowPageMain();
			if (key == KEY_K2) Buzzer_Alarm(ALARM_TEMP_HIGH);
			if (key == KEY_K3) Buzzer_Stop();
			break;
		case PAGE_GPS:  ShowPageGPS();  break;
		case PAGE_SD:
			OLED_ShowString(1, 1, "--SD Card Info--");
			OLED_ShowString(2, 1, "Type:");
			OLED_ShowNum(2, 6, SD_GetType(), 1);
			OLED_ShowString(3, 1, "Size:");
			OLED_ShowNum(3, 6, SD_GetSectorCount() / 2048, 5);
			OLED_ShowString(3, 11, "MB");
			OLED_ShowString(4, 1, "K1:Back       ");
			break;
		case PAGE_WIFI: ShowPageWiFi(); break;
		case PAGE_SYSINFO: ShowPageSysInfo(); break;
		}

		/* 后台: GPS */
		GPS_Poll();
		if (gps.data_ready) GPS_Parse();

		/* DHT11 */
		dht_timer++;
		if (dht_timer >= 100) { dht_timer = 0;
			if (DHT11_ReadData(&temp, &humi) != 0) temp = 0, humi = 0; }

		/* 定时MQTT上传(每10秒) */
		if (mqtt_ok)
		{
			upload_timer++;
			if (upload_timer >= 500)
			{
				upload_timer = 0;
				/* 构造转义JSON: {\"t\":28.5,\"h\":65.3} */
				char cmd[80];
				sprintf(cmd, "AT+MQTTPUB=0,\"%s\",\"{\\\"t\\\":%.1f,\\\"h\\\":%.1f}\",0,0",
				        MQTT_PUB_TOPIC, temp, humi);
				ESP_SendAT(cmd);
				ESP_WaitOK(3000);
			}
		}

		Buzzer_Tick();
		Delay_ms(20);
	}
}

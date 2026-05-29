#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "SHT30.h"

int main(void)
{
	float temperature, humidity;
	uint8_t ret;

	/* OLED 初始化 */
	OLED_Init();
	OLED_Clear();
	OLED_ShowString(1, 1, "SHT30 Init...");

	/* SHT30 初始化 */
	SHT30_Init();
	OLED_ShowString(2, 1, "SHT30 OK");

	Delay_ms(1000);
	OLED_Clear();

	while (1)
	{
		ret = SHT30_ReadData(&temperature, &humidity);

		OLED_ShowString(1, 1, "SHT30 Test");
		if (ret == 0)
		{
			OLED_ShowString(2, 1, "Temp:     C");
			OLED_ShowSignedNum(2, 6, (int32_t)temperature, 2);
			OLED_ShowChar(2, 8, '.');
			OLED_ShowNum(2, 9, (uint32_t)(temperature * 10) % 10, 1);

			OLED_ShowString(3, 1, "Humi:     %");
			OLED_ShowSignedNum(3, 6, (int32_t)humidity, 2);
			OLED_ShowChar(3, 8, '.');
			OLED_ShowNum(3, 9, (uint32_t)(humidity * 10) % 10, 1);

			OLED_ShowString(4, 1, "Status:OK   ");
		}
		else
		{
			OLED_ShowString(2, 1, "Read Error!  ");
			OLED_ShowNum(3, 1, ret, 1);
		}

		Delay_ms(500);
	}
}

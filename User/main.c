#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "DHT11.h"

int main(void)
{
	float t, h;
	uint8_t ret;

	OLED_Init();
	OLED_Clear();
	OLED_ShowString(1, 1, "DHT11 Test PB8");

	DHT11_Init();

	while (1)
	{
		ret = DHT11_ReadData(&t, &h);

		OLED_ShowString(2, 1, "Err:");
		OLED_ShowNum(2, 5, ret, 1);

		if (ret == 0)
		{
			int32_t t_int = (int32_t)t;
			int32_t t_dec = (int32_t)(t * 10) % 10;
			int32_t h_int = (int32_t)h;
			int32_t h_dec = (int32_t)(h * 10) % 10;

			OLED_ShowString(3, 1, "T:");
			OLED_ShowNum(3, 3, t_int, 2);
			OLED_ShowString(3, 5, ".");
			OLED_ShowNum(3, 6, t_dec, 1);
			OLED_ShowString(3, 7, " C");

			OLED_ShowString(4, 1, "H:");
			OLED_ShowNum(4, 3, h_int, 2);
			OLED_ShowString(4, 5, ".");
			OLED_ShowNum(4, 6, h_dec, 1);
			OLED_ShowString(4, 7, " %");
		}
		else
		{
			OLED_ShowString(3, 1, "Read Error");
			OLED_ShowString(4, 1, "Pin=PB8");
		}

		Delay_ms(1500);  /* DHT11两次测量间隔至少1s */
	}
}

#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "Key.h"

int main(void)
{
	uint8_t key;

	OLED_Init();
	OLED_Clear();
	OLED_ShowString(1, 1, "Key Test");

	Key_Init();

	while (1)
	{
		key = Key_Scan();

		OLED_ShowString(2, 1, "PB0:");
		OLED_ShowNum(2, 5, !!(GPIOB->IDR & GPIO_Pin_0), 1);
		OLED_ShowString(2, 7, "PB1:");
		OLED_ShowNum(2, 11, !!(GPIOB->IDR & GPIO_Pin_1), 1);
		OLED_ShowString(2, 13, "PB12:");
		OLED_ShowNum(2, 17, !!(GPIOB->IDR & GPIO_Pin_12) ? 1 : 0, 1);

		OLED_ShowString(3, 1, "Key:");
		OLED_ShowNum(3, 5, key, 1);
		OLED_ShowString(3, 7, "       ");

		if (key == KEY_K1) OLED_ShowString(3, 7, "K1!");
		if (key == KEY_K2) OLED_ShowString(3, 7, "K2!");
		if (key == KEY_K3) OLED_ShowString(3, 7, "K3!");

		OLED_ShowString(4, 1, "Press any key");

		Delay_ms(200);
	}
}

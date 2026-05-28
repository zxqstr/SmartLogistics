#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"

int main(void)
{
	OLED_Init();
	OLED_Clear();

	OLED_ShowString(1, 1, "Smart Logistics");
	OLED_ShowString(2, 1, "OLED Test OK!");
	OLED_ShowString(3, 1, "Temp: 28.5 C");
	OLED_ShowString(4, 1, "Humi: 65.3 %");

	while (1)
	{
	}
}

#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"

int main(void)
{
	/* PC13 LED 指示程序运行 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	/* 先闪一下表示启动 */
	GPIO_ResetBits(GPIOC, GPIO_Pin_13);
	Delay_ms(300);
	GPIO_SetBits(GPIOC, GPIO_Pin_13);
	Delay_ms(300);

	/* OLED 初始化 */
	OLED_Init();
	OLED_Clear();

	OLED_ShowString(1, 1, "Smart Logistics");
	OLED_ShowString(2, 1, "OLED Test OK!");
	OLED_ShowString(3, 1, "Temp:28.5C");
	OLED_ShowString(4, 1, "Humi:65.3%");

	while (1)
	{
		/* LED 心跳闪烁 */
		GPIO_ResetBits(GPIOC, GPIO_Pin_13);
		Delay_ms(500);
		GPIO_SetBits(GPIOC, GPIO_Pin_13);
		Delay_ms(500);
	}
}

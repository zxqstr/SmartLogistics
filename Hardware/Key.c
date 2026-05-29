#include "Key.h"
#include "Delay.h"

/* PB0=K1, PB1=K2, PB12=K3, 内部上拉, 按下为低电平 */

#define KEY_COUNT  3

static const uint16_t key_pins[KEY_COUNT] = {
	GPIO_Pin_0,   /* K1 */
	GPIO_Pin_1,   /* K2 */
	GPIO_Pin_12   /* K3 */
};

static uint16_t key_debounce[KEY_COUNT] = {0};

void Key_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitTypeDef s;
	s.GPIO_Mode = GPIO_Mode_IPU;
	s.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_12;
	s.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &s);
}

uint8_t Key_Scan(void)
{
	uint8_t i;

	for (i = 0; i < KEY_COUNT; i++)
	{
		if (!(GPIOB->IDR & key_pins[i]))  /* 当前按下(低电平) */
		{
			key_debounce[i]++;
			if (key_debounce[i] >= 3)     /* 连续3次(60ms)确认按下 */
			{
				key_debounce[i] = 0;
				return i + 1;
			}
		}
		else
		{
			key_debounce[i] = 0;          /* 未按, 清零计数 */
		}
	}
	return 0;
}

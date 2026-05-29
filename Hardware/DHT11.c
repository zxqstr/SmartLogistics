#include "DHT11.h"
#include "Delay.h"

/* PB8 作为单总线数据引脚 */
#define DQ_LOW()   GPIO_WriteBit(GPIOB, GPIO_Pin_8, Bit_RESET)
#define DQ_HIGH()  GPIO_WriteBit(GPIOB, GPIO_Pin_8, Bit_SET)
#define DQ_READ()  GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_8)

void DHT11_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitTypeDef s;
	s.GPIO_Mode = GPIO_Mode_Out_OD;  /* 开漏输出, 可写可读 */
	s.GPIO_Pin = GPIO_Pin_8;
	s.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &s);

	DQ_HIGH();  /* 释放总线 */
	Delay_ms(1000); /* 等待DHT11上电稳定, 要求最少1s */
}

/* 等待引脚变为指定电平, 超时返回1 */
static uint8_t DHT11_WaitPin(uint8_t level, uint16_t timeout_us)
{
	while (timeout_us--)
	{
		if (DQ_READ() == level) return 0;
		Delay_us(1);
	}
	return 1; /* 超时 */
}

uint8_t DHT11_ReadData(float *temperature, float *humidity)
{
	uint8_t buf[5] = {0};
	uint8_t i, j;

	/* 1. 主机发起始信号: 拉低18ms, 释放20-40us */
	DQ_LOW();
	Delay_ms(18);
	DQ_HIGH();
	Delay_us(30);

	/* 2. 等待DHT11响应 */
	if (DHT11_WaitPin(0, 100)) return 1;  /* 等DHT11拉低 */
	if (DHT11_WaitPin(1, 100)) return 2;  /* 等DHT11释放 */

	/* 3. 读取40位数据 */
	for (j = 0; j < 5; j++)
	{
		for (i = 0; i < 8; i++)
		{
			/* 等起始低电平(50us) */
			if (DHT11_WaitPin(0, 100)) return 3;
			/* 等低电平结束, 进入高电平 */
			if (DHT11_WaitPin(1, 100)) return 4;
			/* 高电平持续26~28us=bit0, 70us=bit1, 在30us处采样 */
			Delay_us(40);  /* bit0:26~28us已结束, bit1:70us仍为高 */
			buf[j] <<= 1;
			if (DQ_READ()) buf[j] |= 1;
			/* 等当前bit高电平结束, 回到低电平(最后一位之后不需要等) */
			if (i < 7 || j < 4) {
				if (DHT11_WaitPin(0, 100)) return 5;
			}
		}
	}

	/* 4. 校验 */
	if ((buf[0] + buf[1] + buf[2] + buf[3]) != buf[4]) return 5;

	/* 5. 提取温湿度(DHT11小数部分固定为0) */
	*humidity    = (float)buf[0];
	*temperature = (float)buf[2];

	return 0;
}

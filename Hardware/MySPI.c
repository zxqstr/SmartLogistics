#include "MySPI.h"
#include "Delay.h"

/* PA4=CS, PA5=SCK, PA6=MISO, PA7=MOSI */

#define CS_LOW()   GPIO_ResetBits(GPIOA, GPIO_Pin_4)
#define CS_HIGH()  GPIO_SetBits(GPIOA, GPIO_Pin_4)
#define SCK_LOW()  GPIO_ResetBits(GPIOA, GPIO_Pin_5)
#define SCK_HIGH() GPIO_SetBits(GPIOA, GPIO_Pin_5)
#define MOSI_LOW()  GPIO_ResetBits(GPIOA, GPIO_Pin_7)
#define MOSI_HIGH() GPIO_SetBits(GPIOA, GPIO_Pin_7)
#define MISO_READ  GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6)

void MySPI_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitTypeDef s;

	/* PA4=CS, PA5=SCK, PA7=MOSI: 推挽输出 */
	s.GPIO_Mode = GPIO_Mode_Out_PP;
	s.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_7;
	s.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &s);

	/* PA6=MISO: 上拉输入 */
	s.GPIO_Mode = GPIO_Mode_IPU;
	s.GPIO_Pin = GPIO_Pin_6;
	GPIO_Init(GPIOA, &s);

	CS_HIGH();
	SCK_HIGH();
}

void MySPI_Start(void)
{
	CS_LOW();
}

void MySPI_Stop(void)
{
	CS_HIGH();
}

uint8_t MySPI_SwapByte(uint8_t txByte)
{
	uint8_t i, rxByte = 0;

	for (i = 0; i < 8; i++)
	{
		/* 主机在上升沿采样, 先设MOSI再发SCK脉冲 */
		if (txByte & 0x80)
			MOSI_HIGH();
		else
			MOSI_LOW();
		txByte <<= 1;

		SCK_LOW();
		Delay_us(1);
		SCK_HIGH();
		Delay_us(1);

		/* 读取MISO */
		rxByte <<= 1;
		if (MISO_READ)
			rxByte |= 1;
	}

	SCK_LOW();
	return rxByte;
}

/* CS保持高, 发送哑时钟(SD卡上电初始化需要) */
void MySPI_SendDummyClocks(uint16_t count)
{
	CS_HIGH();
	while (count--)
	{
		SCK_LOW();
		Delay_us(10);
		SCK_HIGH();
		Delay_us(10);
	}
}

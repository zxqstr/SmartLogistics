#include "GPS.h"
#include <string.h>
#include <stdlib.h>

GPS_Data_t gps = {0};

#define RX_BUF_SIZE  256

static char rx_buf[RX_BUF_SIZE];
static volatile uint16_t rx_idx = 0;
static char nmea_buf[128];

void GPS_Init(void)
{
	GPIO_InitTypeDef gpio;
	USART_InitTypeDef usart;

	/* GPIOA + USART1 时钟 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

	/* PA9=TX(复用推挽), PA10=RX(浮空输入) */
	gpio.GPIO_Mode = GPIO_Mode_AF_PP;
	gpio.GPIO_Pin = GPIO_Pin_9;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio);

	gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	gpio.GPIO_Pin = GPIO_Pin_10;
	GPIO_Init(GPIOA, &gpio);

	/* USART1: 9600-8-N-1, APB2=72MHz */
	usart.USART_BaudRate = 9600;
	usart.USART_WordLength = USART_WordLength_8b;
	usart.USART_StopBits = USART_StopBits_1;
	usart.USART_Parity = USART_Parity_No;
	usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_Init(USART1, &usart);

	USART_Cmd(USART1, ENABLE);
}

/* 轮询接收 */
uint8_t GPS_Poll(void)
{
	uint8_t count = 0;
	char ch;

	while (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != RESET)
	{
		ch = USART_ReceiveData(USART1);
		count++;
		gps.rx_count++;

		if (ch == '$')
			rx_idx = 0;

		if (rx_idx < RX_BUF_SIZE - 1)
			rx_buf[rx_idx++] = ch;

		if (ch == '\n' && rx_idx > 6)
		{
			rx_buf[rx_idx] = '\0';
			if (rx_buf[0] == '$' &&
			    rx_buf[3] == 'R' && rx_buf[4] == 'M' && rx_buf[5] == 'C')
			{
				if (rx_idx < sizeof(nmea_buf))
				{
					memcpy(nmea_buf, rx_buf, rx_idx);
					nmea_buf[rx_idx] = '\0';
					gps.data_ready = 1;
				}
			}
			rx_idx = 0;
		}
	}
	return count;
}

static float NMEA2Decimal(const char *nmea, char dir)
{
	float val, deg, min;

	val = (float)atof(nmea);

	if (dir == 'N' || dir == 'S')
	{
		deg = (float)((int)(val / 100.0f));
		min = val - deg * 100.0f;
	}
	else
	{
		deg = (float)((int)(val / 100.0f));
		min = val - deg * 100.0f;
	}

	val = deg + min / 60.0f;

	if (dir == 'S' || dir == 'W')
		val = -val;

	return val;
}

void GPS_Parse(void)
{
	char *p, *fields[13];
	uint8_t i;

	if (!gps.data_ready) return;
	gps.data_ready = 0;

	p = nmea_buf;
	for (i = 0; i < 13 && p; i++)
	{
		fields[i] = p;
		p = strchr(p, ',');
		if (p) { *p = '\0'; p++; }
	}

	if (fields[2] && fields[2][0] == 'A')
		gps.valid = 1;
	else
	{
		gps.valid = 0;
		return;
	}

	if (fields[1]) strncpy(gps.utc_time, fields[1], 11);
	if (fields[3] && fields[4])
		gps.latitude = NMEA2Decimal(fields[3], fields[4][0]);
	if (fields[5] && fields[6])
		gps.longitude = NMEA2Decimal(fields[5], fields[6][0]);
	if (fields[7]) gps.speed_kn = (float)atof(fields[7]);
}

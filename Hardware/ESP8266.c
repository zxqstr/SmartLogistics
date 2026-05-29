#include "ESP8266.h"
#include "Delay.h"
#include <string.h>
#include <stdio.h>

/* USART1: PA9=TX, PA10=RX, 115200bps */

#define RX_BUF_SIZE  512
static volatile char rx_ring[RX_BUF_SIZE];
static volatile uint16_t rx_head = 0, rx_tail = 0;

static uint8_t  esp_state = ESP_STATE_IDLE;
static char     cmd_buf[128];
static volatile uint8_t cmd_ok = 0;

/* ---- 环形缓冲 ---- */
static char ring_read(void)
{
	if (rx_head == rx_tail) return 0;
	char ch = rx_ring[rx_tail];
	rx_tail = (rx_tail + 1) % RX_BUF_SIZE;
	return ch;
}

void USART1_IRQHandler(void)
{
	if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		char ch = USART_ReceiveData(USART1);
		uint16_t next = (rx_head + 1) % RX_BUF_SIZE;
		if (next != rx_tail)
		{
			rx_ring[rx_head] = ch;
			rx_head = next;
		}

		/* 状态机检测 "OK\r\n" 序列 */
		static uint8_t ok_st = 0;
		switch (ok_st)
		{
		case 0: if (ch == '\r') ok_st = 1; break;
		case 1: if (ch == '\n') ok_st = 2; break;
		case 2: if (ch == 'O') ok_st = 3; else if (ch == '\r') ok_st = 1; else ok_st = 0; break;
		case 3: if (ch == 'K') ok_st = 4; else if (ch == '\r') ok_st = 1; else ok_st = 0; break;
		case 4: if (ch == '\r') ok_st = 5; else ok_st = 0; break;
		case 5: if (ch == '\n') { cmd_ok = 1; } ok_st = 0; break;
		}
	}
}

void ESP8266_Init(void)
{
	GPIO_InitTypeDef gpio;
	USART_InitTypeDef usart;
	NVIC_InitTypeDef nvic;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

	gpio.GPIO_Mode = GPIO_Mode_AF_PP;
	gpio.GPIO_Pin = GPIO_Pin_9;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio);

	gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	gpio.GPIO_Pin = GPIO_Pin_10;
	GPIO_Init(GPIOA, &gpio);

	usart.USART_BaudRate = 115200;
	usart.USART_WordLength = USART_WordLength_8b;
	usart.USART_StopBits = USART_StopBits_1;
	usart.USART_Parity = USART_Parity_No;
	usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_Init(USART1, &usart);

	nvic.NVIC_IRQChannel = USART1_IRQn;
	nvic.NVIC_IRQChannelPreemptionPriority = 1;
	nvic.NVIC_IRQChannelSubPriority = 0;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic);
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

	USART_Cmd(USART1, ENABLE);
	Delay_ms(1000);
}

void ESP_SendAT(const char *cmd)
{
	while (*cmd) {
		while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
		USART_SendData(USART1, *cmd++);
	}
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
	USART_SendData(USART1, '\r');
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
	USART_SendData(USART1, '\n');
}

uint8_t ESP_WaitOK(uint16_t timeout_ms)
{
	uint16_t t = 0;
	cmd_ok = 0;
	while (!cmd_ok && t < timeout_ms)
	{
		Delay_ms(10);
		t += 10;
	}
	/* 清空缓冲 */
	while (rx_head != rx_tail) ring_read();
	return cmd_ok ? 0 : 1;
}

uint8_t ESP_ConnectWiFi(const char *ssid, const char *pwd)
{
	/* STA模式 */
	ESP_SendAT("AT+CWMODE=1");
	if (ESP_WaitOK(2000)) return 1;

	/* 连WiFi */
	sprintf(cmd_buf, "AT+CWJAP=\"%s\",\"%s\"", ssid, pwd);
	ESP_SendAT(cmd_buf);
	if (ESP_WaitOK(15000)) return 2;

	esp_state = ESP_STATE_WIFI_CONN;
	return 0;
}

uint8_t ESP_MQTT_Config(const char *client_id, const char *user, const char *pass)
{
	sprintf(cmd_buf, "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"",
	        client_id, user, pass);
	ESP_SendAT(cmd_buf);
	if (ESP_WaitOK(3000)) return 1;
	return 0;
}

uint8_t ESP_MQTT_Connect(const char *host, uint16_t port)
{
	sprintf(cmd_buf, "AT+MQTTCONN=0,\"%s\",%d,1", host, port);
	ESP_SendAT(cmd_buf);
	if (ESP_WaitOK(10000)) return 1;

	esp_state = ESP_STATE_MQTT_CONN;
	return 0;
}

uint8_t ESP_MQTT_Publish(const char *topic, const char *data)
{
	sprintf(cmd_buf, "AT+MQTTPUB=0,\"%s\",\"%s\",0,0", topic, data);
	ESP_SendAT(cmd_buf);
	return ESP_WaitOK(5000);
}

uint8_t ESP_MQTT_Subscribe(const char *topic)
{
	sprintf(cmd_buf, "AT+MQTTSUB=0,\"%s\",0", topic);
	ESP_SendAT(cmd_buf);
	return ESP_WaitOK(3000);
}

void ESP_Process(void) { /* ISR处理接收 */ }
uint8_t ESP_GetState(void) { return esp_state; }

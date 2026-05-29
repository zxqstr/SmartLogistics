#include "MQTT.h"
#include "ESP8266.h"
#include "Delay.h"
#include <string.h>
#include <stdio.h>

static MQTT_Config_t cfg;
static uint8_t state = MQTT_DISCONNECTED;
static uint16_t packet_id = 1;
static uint32_t last_ping = 0;

void MQTT_SetConfig(const MQTT_Config_t *c) { memcpy(&cfg, c, sizeof(cfg)); }
uint8_t MQTT_GetState(void) { return state; }

/* MQTT剩余长度编码 */
static uint8_t MQTT_EncodeLen(uint32_t len, uint8_t *buf)
{
	uint8_t i = 0;
	do {
		buf[i] = len % 128;
		len /= 128;
		if (len > 0) buf[i] |= 0x80;
		i++;
	} while (len > 0 && i < 4);
	return i;
}

/* 写大端uint16 */
static void WriteU16(uint8_t *buf, uint16_t val)
{
	buf[0] = (val >> 8) & 0xFF;
	buf[1] = val & 0xFF;
}

/* 构建CONNECT报文 */
static uint16_t MQTT_BuildConnect(uint8_t *buf)
{
	uint8_t *p = buf;
	uint16_t cid_len = strlen(cfg.client_id);
	uint16_t user_len = cfg.username ? strlen(cfg.username) : 0;
	uint16_t pass_len = cfg.password ? strlen(cfg.password) : 0;

	/* 可变头: 协议名 + 协议级别 + 标志 + 保活 */
	uint16_t var_len = 2 + 4 + 1 + 1 + 2;  /* MQTT + 0x04 + flags + keepalive */
	var_len += 2 + cid_len;                 /* client ID */
	var_len += 2 + user_len;                /* username */
	var_len += 2 + pass_len;                /* password */

	/* 固定头 */
	*p++ = 0x10;  /* CONNECT */
	p += MQTT_EncodeLen(var_len, p);

	/* 协议名 "MQTT" */
	WriteU16(p, 4); p += 2;
	*p++ = 'M'; *p++ = 'Q'; *p++ = 'T'; *p++ = 'T';

	/* 协议级别 */
	*p++ = 0x04;  /* 3.1.1 */

	/* 连接标志 */
	uint8_t flags = 0xC2;  /* 用户名+密码, 清除会话 */
	*p++ = flags;

	/* 保活时间 */
	WriteU16(p, cfg.keepalive); p += 2;

	/* 客户端ID */
	WriteU16(p, cid_len); p += 2;
	memcpy(p, cfg.client_id, cid_len); p += cid_len;

	/* 用户名 */
	WriteU16(p, user_len); p += 2;
	if (user_len) { memcpy(p, cfg.username, user_len); p += user_len; }

	/* 密码 */
	WriteU16(p, pass_len); p += 2;
	if (pass_len) { memcpy(p, cfg.password, pass_len); p += pass_len; }

	return p - buf;
}

/* 构建PUBLISH报文(QoS0) */
static uint16_t MQTT_BuildPublish(uint8_t *buf, const char *topic, const char *payload)
{
	uint8_t *p = buf;
	uint16_t t_len = strlen(topic);
	uint16_t p_len = strlen(payload);
	uint16_t rem_len = 2 + t_len + p_len;

	*p++ = 0x30;  /* PUBLISH, QoS0 */
	p += MQTT_EncodeLen(rem_len, p);

	WriteU16(p, t_len); p += 2;
	memcpy(p, topic, t_len); p += t_len;
	memcpy(p, payload, p_len); p += p_len;

	return p - buf;
}

/* 构建SUBSCRIBE报文 */
static uint16_t MQTT_BuildSubscribe(uint8_t *buf, uint16_t pkt_id, const char *topic)
{
	uint8_t *p = buf;
	uint16_t t_len = strlen(topic);
	uint16_t rem_len = 2 + 2 + t_len + 1;  /* pkt_id + topic_len + topic + qos */

	*p++ = 0x82;  /* SUBSCRIBE */
	p += MQTT_EncodeLen(rem_len, p);

	WriteU16(p, pkt_id); p += 2;
	WriteU16(p, t_len); p += 2;
	memcpy(p, topic, t_len); p += t_len;
	*p++ = 0x00;  /* QoS0 */

	return p - buf;
}

/* 构建PINGREQ */
static uint16_t MQTT_BuildPingReq(uint8_t *buf)
{
	buf[0] = 0xC0;
	buf[1] = 0x00;
	return 2;
}

/* 构建DISCONNECT */
static uint16_t MQTT_BuildDisconnect(uint8_t *buf)
{
	buf[0] = 0xE0;
	buf[1] = 0x00;
	return 2;
}

/* ---- 公开接口 ---- */

void MQTT_Init(const MQTT_Config_t *c)
{
	MQTT_SetConfig(c);
	ESP8266_Init(115200);
	Delay_ms(500);
}

uint8_t MQTT_Connect(void)
{
	uint8_t buf[256];
	uint16_t len;

	/* 1. 测AT */
	if (ESP_Test() != ESP_OK) return 1;

	/* 2. STA模式 */
	if (ESP_SetMode(1) != ESP_OK) return 2;

	/* 3. 连WiFi(需提前配置SSID/密码) */
	/* 在实际使用中, SSID和密码应由用户输入或预设 */
	/* 这里先只做TCP连接测试 */

	return 0;
}

uint8_t MQTT_ConnectWiFi(const char *ssid, const char *pwd)
{
	/* STA模式 */
	if (ESP_SetMode(1) != ESP_OK) return 1;

	/* 连WiFi */
	if (ESP_ConnectWiFi(ssid, pwd) != ESP_OK) return 2;

	/* 连MQTT Broker */
	if (ESP_ConnectTCP(cfg.broker_ip, cfg.broker_port) != ESP_OK) return 3;

	/* 发CONNECT */
	uint8_t buf[256];
	uint16_t len = MQTT_BuildConnect(buf);
	if (ESP_SendData(buf, len) != ESP_OK) return 4;

	Delay_ms(500);

	/* 订阅 */
	len = MQTT_BuildSubscribe(buf, packet_id++, cfg.sub_topic);
	if (ESP_SendData(buf, len) != ESP_OK) return 5;

	state = MQTT_CONNECTED;
	last_ping = 0;

	return 0;
}

uint8_t MQTT_Publish(const char *payload)
{
	uint8_t buf[512];
	if (state != MQTT_CONNECTED) return 1;

	uint16_t len = MQTT_BuildPublish(buf, cfg.pub_topic, payload);
	return ESP_SendData(buf, len);
}

uint8_t MQTT_Disconnect(void)
{
	uint8_t buf[4];
	uint16_t len = MQTT_BuildDisconnect(buf);
	ESP_SendData(buf, len);
	ESP_CloseTCP();
	state = MQTT_DISCONNECTED;
	return 0;
}

void MQTT_Process(void)
{
	if (state != MQTT_CONNECTED) return;

	/* 心跳: 保活时间的2/3发PING */
	uint32_t interval = (uint32_t)cfg.keepalive * 1000 * 2 / 3;
	uint32_t now = 0;  /* TODO: 使用系统时钟 */

	if (last_ping && (now - last_ping) > interval)
	{
		uint8_t buf[4];
		MQTT_BuildPingReq(buf);
		ESP_SendData(buf, 2);
		last_ping = now;
	}
}

#ifndef __MQTT_H
#define __MQTT_H

#include "stm32f10x.h"

/* MQTT配置 */
typedef struct {
	const char *broker_ip;    /* 服务器IP或域名 */
	uint16_t   broker_port;  /* 端口, 默认1883 */
	const char *client_id;
	const char *username;
	const char *password;
	const char *pub_topic;    /* 发布主题 */
	const char *sub_topic;    /* 订阅主题 */
	uint16_t   keepalive;    /* 心跳间隔(秒) */
} MQTT_Config_t;

/* 状态 */
#define MQTT_DISCONNECTED  0
#define MQTT_CONNECTING    1
#define MQTT_CONNECTED     2

void MQTT_Init(const MQTT_Config_t *cfg);
void MQTT_SetConfig(const MQTT_Config_t *cfg);

/* 连接与维护 */
uint8_t MQTT_Connect(void);       /* 建立MQTT连接 */
uint8_t MQTT_Publish(const char *payload);  /* 发布消息 */
uint8_t MQTT_Disconnect(void);

/* 主循环调用: 心跳维持+接收分发 */
void MQTT_Process(void);

uint8_t MQTT_GetState(void);

#endif

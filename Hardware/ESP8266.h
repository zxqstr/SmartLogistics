#ifndef __ESP8266_H
#define __ESP8266_H

#include "stm32f10x.h"

/* 连接状态 */
#define ESP_STATE_IDLE        0
#define ESP_STATE_WIFI_CONN   1
#define ESP_STATE_MQTT_CONN   2
#define ESP_STATE_READY       3

void ESP8266_Init(void);
uint8_t ESP_ConnectWiFi(const char *ssid, const char *pwd);
uint8_t ESP_MQTT_Config(const char *client_id, const char *user, const char *pass);
uint8_t ESP_MQTT_Connect(const char *host, uint16_t port);
uint8_t ESP_MQTT_Publish(const char *topic, const char *data);
uint8_t ESP_MQTT_Subscribe(const char *topic);
void ESP_Process(void);
uint8_t ESP_GetState(void);
void ESP_SendAT(const char *cmd);
uint8_t ESP_WaitOK(uint16_t timeout_ms);

#endif

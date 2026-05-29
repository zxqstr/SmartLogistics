#ifndef __GPS_H
#define __GPS_H

#include "stm32f10x.h"

typedef struct {
	uint8_t  valid;        /* A=有效, V=无效 */
	char     utc_time[12]; /* hhmmss.sss */
	float    latitude;     /* 十进制度 */
	float    longitude;
	float    speed_kn;     /* 速度(节) */
	uint8_t  data_ready;   /* 有新数据 */
	uint16_t rx_count;     /* 收到的字节总数 */
} GPS_Data_t;

extern GPS_Data_t gps;

void GPS_Init(void);
void GPS_Parse(void);
uint8_t GPS_Poll(void);    /* 轮询接收, 返回新收到的字节数 */

#endif

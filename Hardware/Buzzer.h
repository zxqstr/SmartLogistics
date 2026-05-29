#ifndef __BUZZER_H
#define __BUZZER_H

#include "stm32f10x.h"

/* 报警类型 */
typedef enum {
	ALARM_NONE = 0,
	ALARM_TEMP_HIGH,      /* 短鸣3声 */
	ALARM_TEMP_LOW,       /* 短鸣2声 */
	ALARM_HUMI_HIGH,      /* 短鸣3声 */
	ALARM_HUMI_LOW,       /* 短鸣2声 */
	ALARM_SD_FAULT,       /* 长鸣1声 */
	ALARM_WIFI_DISCONN,   /* 间歇短鸣 */
	ALARM_GPS_NO_FIX,     /* 间歇长鸣 */
} AlarmType_t;

void Buzzer_Init(void);
void Buzzer_Alarm(AlarmType_t type);   /* 触发报警, 调用一次即可 */
void Buzzer_Stop(void);                /* 停止报警 */
void Buzzer_Tick(void);                /* 主循环每1ms调用一次 */

#endif

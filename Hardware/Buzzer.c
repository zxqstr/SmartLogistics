#include "Buzzer.h"

/* PB3, 有源蜂鸣器高电平触发 */

#define BUZZER_ON()   GPIO_ResetBits(GPIOB, GPIO_Pin_15)  /* 低电平触发 */
#define BUZZER_OFF()  GPIO_SetBits(GPIOB, GPIO_Pin_15)

typedef struct {
	uint16_t on_ms;       /* 鸣响时长(ms) */
	uint16_t off_ms;      /* 静音时长(ms) */
	uint8_t  repeat;      /* 重复次数, 0=无限 */
} AlarmPattern_t;

static const AlarmPattern_t patterns[] = {
	[ALARM_NONE]         = {0,   0,   0},
	[ALARM_TEMP_HIGH]    = {200, 200, 3},
	[ALARM_TEMP_LOW]     = {200, 200, 2},
	[ALARM_HUMI_HIGH]    = {200, 200, 3},
	[ALARM_HUMI_LOW]     = {200, 200, 2},
	[ALARM_SD_FAULT]     = {1000, 500, 1},
	[ALARM_WIFI_DISCONN] = {100, 900, 0},  /* 无限 */
	[ALARM_GPS_NO_FIX]   = {500, 500, 0},  /* 无限 */
};

static AlarmType_t current_alarm = ALARM_NONE;
static uint16_t tick_counter = 0;
static uint8_t repeat_count = 0;
static uint8_t phase = 0;  /* 0=鸣响, 1=静音 */

void Buzzer_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitTypeDef s;
	s.GPIO_Mode = GPIO_Mode_Out_PP;
	s.GPIO_Pin = GPIO_Pin_15;
	s.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &s);
	BUZZER_OFF();
}

void Buzzer_Alarm(AlarmType_t type)
{
	if (type == ALARM_NONE) return;
	current_alarm = type;
	tick_counter = 0;
	repeat_count = 0;
	phase = 0;
	BUZZER_ON();  /* 开始鸣响 */
}

void Buzzer_Stop(void)
{
	current_alarm = ALARM_NONE;
	BUZZER_OFF();
}

void Buzzer_Tick(void)
{
	const AlarmPattern_t *pat;

	if (current_alarm == ALARM_NONE) return;

	pat = &patterns[current_alarm];

	if (phase == 0)  /* 鸣响阶段 */
	{
		if (++tick_counter >= pat->on_ms)
		{
			tick_counter = 0;
			phase = 1;
			BUZZER_OFF();
		}
	}
	else  /* 静音阶段 */
	{
		if (++tick_counter >= pat->off_ms)
		{
			tick_counter = 0;
			phase = 0;

			if (pat->repeat > 0)  /* 有限次重复 */
			{
				repeat_count++;
				if (repeat_count >= pat->repeat)
				{
					current_alarm = ALARM_NONE;
					return;
				}
			}
			BUZZER_ON();  /* 开始下一轮 */
		}
	}
}

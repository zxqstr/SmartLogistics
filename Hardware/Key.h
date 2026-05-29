#ifndef __KEY_H
#define __KEY_H

#include "stm32f10x.h"

#define KEY_K1  1
#define KEY_K2  2
#define KEY_K3  3

/* PB0=K1, PB1=K2, PB12=K3 */

void Key_Init(void);
uint8_t Key_Scan(void);  /* 返回按键号(1/2/3), 无按键返回0 */

#endif

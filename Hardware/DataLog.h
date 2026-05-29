#ifndef __DATALOG_H
#define __DATALOG_H

#include "stm32f10x.h"

uint8_t DataLog_Init(void);
uint8_t DataLog_Write(float temp, float humi,
                      float lat, float lon, float speed,
                      uint8_t gps_valid);

#endif

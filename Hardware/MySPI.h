#ifndef __MYSPI_H
#define __MYSPI_H

#include "stm32f10x.h"

void MySPI_Init(void);
void MySPI_Start(void);     /* CS拉低 */
void MySPI_Stop(void);      /* CS拉高 */
uint8_t MySPI_SwapByte(uint8_t txByte);
void MySPI_SendDummyClocks(uint16_t count);  /* CS高时发哑时钟 */

#endif

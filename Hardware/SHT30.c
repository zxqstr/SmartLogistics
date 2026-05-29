#include "SHT30.h"
#include "Delay.h"
#include "stm32f10x.h"

/* SHT30 I2C 地址: 0x44 (ADDR=GND) 或 0x45 (ADDR=VDD) */
static uint8_t i2cAddr = 0x45;

/* ---- 硬件抽象层 (照搬Sensirion官方代码: PB13=SCL, PB14=SDA) ---- */

#define SDA_LOW()   (GPIOB->BSRR = 0x40000000)  /* PB14 复位 */
#define SDA_OPEN()  (GPIOB->BSRR = 0x00004000)  /* PB14 置位(开漏释放) */
#define SCL_LOW()   (GPIOB->BSRR = 0x20000000)  /* PB13 复位 */
#define SCL_OPEN()  (GPIOB->BSRR = 0x00002000)  /* PB13 置位(开漏释放) */
#define SDA_READ    (GPIOB->IDR & 0x4000)       /* 读PB14 */
#define SCL_READ    (GPIOB->IDR & 0x2000)       /* 读PB13 */

static void I2c_Init(void)
{
	RCC->APB2ENR |= 0x00000008;   /* 使能GPIOB时钟 */
	SDA_OPEN();
	SCL_OPEN();
	GPIOB->CRH &= 0xF00FFFFF;     /* PB13, PB14 开漏输出 */
	GPIOB->CRH |= 0x05500000;
}

/* 等待时钟拉伸结束 */
static uint8_t I2c_WaitClockStretch(uint8_t timeout)
{
	while (SCL_READ == 0)
	{
		if (timeout-- == 0) return 1;
		Delay_us(1000);
	}
	return 0;
}

static void I2c_Start(void)
{
	SDA_OPEN();
	Delay_us(1);
	SCL_OPEN();
	Delay_us(1);
	SDA_LOW();
	Delay_us(10);
	SCL_LOW();
	Delay_us(10);
}

static void I2c_Stop(void)
{
	SCL_LOW();
	Delay_us(1);
	SDA_LOW();
	Delay_us(1);
	SCL_OPEN();
	Delay_us(10);
	SDA_OPEN();
	Delay_us(10);
}

static uint8_t I2c_WriteByte(uint8_t txByte)
{
	uint8_t mask, error;
	for (mask = 0x80; mask > 0; mask >>= 1)
	{
		if (mask & txByte)
			SDA_OPEN();
		else
			SDA_LOW();
		SCL_OPEN();
		Delay_us(1);
		Delay_us(5);
		SCL_LOW();
		Delay_us(1);
	}
	SDA_OPEN();
	SCL_OPEN();
	Delay_us(1);
	error = (SDA_READ != 0) ? 1 : 0;  /* 从机未应答=1 */
	SCL_LOW();
	Delay_us(20);
	return error;
}

static uint8_t I2c_ReadByte(uint8_t *rxByte, uint8_t ack, uint8_t timeout)
{
	uint8_t mask;
	*rxByte = 0x00;
	SDA_OPEN();
	for (mask = 0x80; mask > 0; mask >>= 1)
	{
		SCL_OPEN();
		Delay_us(1);
		if (I2c_WaitClockStretch(timeout)) return 1;
		Delay_us(3);
		if (SDA_READ) *rxByte |= mask;
		SCL_LOW();
		Delay_us(1);
	}
	if (ack == 0)
		SDA_LOW();
	else
		SDA_OPEN();
	Delay_us(1);
	SCL_OPEN();
	Delay_us(5);
	SCL_LOW();
	SDA_OPEN();
	Delay_us(20);
	return 0;
}

/* ---- SHT3x 传感器层 ---- */

/* CRC-8 多项式: x^8 + x^5 + x^4 + 1 */
#define CRC_POLYNOMIAL  0x131

static uint8_t SHT3X_CalcCrc(uint8_t data[], uint8_t nbrOfBytes)
{
	uint8_t bit, crc = 0xFF;
	uint8_t byteCtr;
	for (byteCtr = 0; byteCtr < nbrOfBytes; byteCtr++)
	{
		crc ^= data[byteCtr];
		for (bit = 8; bit > 0; --bit)
		{
			if (crc & 0x80)
				crc = (crc << 1) ^ CRC_POLYNOMIAL;
			else
				crc = (crc << 1);
		}
	}
	return crc;
}

static uint8_t SHT3X_CheckCrc(uint8_t data[], uint8_t nbrOfBytes, uint8_t checksum)
{
	return (SHT3X_CalcCrc(data, nbrOfBytes) != checksum);
}

static uint8_t SHT3X_WriteCommand(uint16_t command)
{
	uint8_t err = 0;
	err  = I2c_WriteByte(command >> 8);
	err |= I2c_WriteByte(command & 0xFF);
	return err;
}

static uint8_t SHT3X_Read2BytesAndCrc(uint16_t *data, uint8_t finalAck, uint8_t timeout)
{
	uint8_t bytes[2], checksum;
	if (I2c_ReadByte(&bytes[0], 0, timeout)) return 1;
	if (I2c_ReadByte(&bytes[1], 0, 0))       return 1;
	if (I2c_ReadByte(&checksum,  finalAck, 0)) return 1;
	if (SHT3X_CheckCrc(bytes, 2, checksum))   return 2;
	*data = (bytes[0] << 8) | bytes[1];
	return 0;
}

/* ---- 公开接口 ---- */

/* 通用调用复位: 地址0x00+命令0x06, 无需知道设备地址 */
static uint8_t I2c_GeneralCallReset(void)
{
	uint8_t error;
	I2c_Start();
	error  = I2c_WriteByte(0x00);   /* 通用调用地址 */
	error |= I2c_WriteByte(0x06);   /* 复位命令 */
	I2c_Stop();
	return error;
}

void SHT30_Init(void)
{
	I2c_Init();
	Delay_ms(10);
	I2c_GeneralCallReset();   /* 复位总线上的SHT30 */
	Delay_ms(50);             /* 等待复位完成 */
}

static uint8_t SHT30_TryRead(uint8_t addr, float *t, float *h)
{
	uint8_t error;
	uint16_t rawTemp, rawHumi;

	/* 1. 写入测量命令 */
	I2c_Start();
	error  = I2c_WriteByte(addr << 1);
	if (error) { I2c_Stop(); return 1; }
	error = SHT3X_WriteCommand(0x2C06);
	if (error) { I2c_Stop(); return 1; }

	/* 2. 启动读访问 */
	I2c_Start();
	error = I2c_WriteByte((addr << 1) | 0x01);
	if (error) { I2c_Stop(); return 1; }

	/* 3. 读取温度+湿度 */
	error  = SHT3X_Read2BytesAndCrc(&rawTemp, 0, 50);
	if (error == 0)
		error = SHT3X_Read2BytesAndCrc(&rawHumi, 1, 0);
	I2c_Stop();

	if (error) return 1;

	*t = -45.0f + 175.0f * rawTemp / 65535.0f;
	*h = 100.0f * rawHumi / 65535.0f;

	return 0;
}

uint8_t SHT30_ReadData(float *temperature, float *humidity)
{
	/* 先试0x44, 再试0x45 */
	if (SHT30_TryRead(0x44, temperature, humidity) == 0) return 0;
	if (SHT30_TryRead(0x45, temperature, humidity) == 0) return 0;
	return 1;
}

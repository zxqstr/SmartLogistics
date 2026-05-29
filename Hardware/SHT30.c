#include "SHT30.h"
#include "Delay.h"

/* SHT30 I2C 地址: 0x44 -> 写0x88, 读0x89 */
#define SHT30_ADDR_W   0x88
#define SHT30_ADDR_R   0x89

/* 单次测量命令: 高重复性, 允许时钟拉伸 */
#define SHT30_CMD_MSB  0x2C
#define SHT30_CMD_LSB  0x06

/* ---- 软件I2C层 (PB8=SCL, PB9=SDA) ---- */

#define SHT30_SCL(x)  GPIO_WriteBit(GPIOB, GPIO_Pin_8, (BitAction)(x))
#define SHT30_SDA(x)  GPIO_WriteBit(GPIOB, GPIO_Pin_9, (BitAction)(x))
#define SHT30_READ()  GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_9)

static void SHT30_I2C_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_SetBits(GPIOB, GPIO_Pin_8 | GPIO_Pin_9);
}

static void I2C_Start(void)
{
	SHT30_SDA(1);
	SHT30_SCL(1);
	SHT30_SDA(0);
	SHT30_SCL(0);
}

static void I2C_Stop(void)
{
	SHT30_SDA(0);
	SHT30_SCL(1);
	SHT30_SDA(1);
}

static void I2C_SendByte(uint8_t Byte)
{
	uint8_t i;
	for (i = 0; i < 8; i++)
	{
		SHT30_SDA(!!(Byte & (0x80 >> i)));
		SHT30_SCL(1);
		SHT30_SCL(0);
	}
}

static uint8_t I2C_ReceiveByte(void)
{
	uint8_t i, Byte = 0x00;
	SHT30_SDA(1);
	for (i = 0; i < 8; i++)
	{
		SHT30_SCL(1);
		if (SHT30_READ()) Byte |= (0x80 >> i);
		SHT30_SCL(0);
	}
	return Byte;
}

static uint8_t I2C_WaitAck(void)
{
	uint8_t ack;
	SHT30_SDA(1);
	SHT30_SCL(1);
	ack = SHT30_READ();
	SHT30_SCL(0);
	return ack;
}

static void I2C_SendAck(uint8_t ack)
{
	SHT30_SDA(ack);
	SHT30_SCL(1);
	SHT30_SCL(0);
}

/* ---- SHT30 驱动层 ---- */

static uint8_t SHT30_CRC8(const uint8_t *data, uint8_t len)
{
	uint8_t crc = 0xFF;
	uint8_t i, j;

	for (i = 0; i < len; i++)
	{
		crc ^= data[i];
		for (j = 0; j < 8; j++)
		{
			if (crc & 0x80)
				crc = (crc << 1) ^ 0x31;
			else
				crc <<= 1;
		}
	}
	return crc;
}

void SHT30_Init(void)
{
	SHT30_I2C_Init();
}

uint8_t SHT30_ReadData(float *temperature, float *humidity)
{
	uint8_t buf[6];
	uint16_t raw_temp, raw_humi;

	/* 发送测量命令 */
	I2C_Start();
	I2C_SendByte(SHT30_ADDR_W);
	if (I2C_WaitAck()) { I2C_Stop(); return 1; }
	I2C_SendByte(SHT30_CMD_MSB);
	if (I2C_WaitAck()) { I2C_Stop(); return 2; }
	I2C_SendByte(SHT30_CMD_LSB);
	if (I2C_WaitAck()) { I2C_Stop(); return 3; }
	I2C_Stop();

	/* 等待测量完成: SHT30典型值20ms */
	Delay_ms(20);

	/* 读取6字节数据 */
	I2C_Start();
	I2C_SendByte(SHT30_ADDR_R);
	if (I2C_WaitAck()) { I2C_Stop(); return 4; }
	buf[0] = I2C_ReceiveByte();  /* 温度高字节 */
	I2C_SendAck(0);
	buf[1] = I2C_ReceiveByte();  /* 温度低字节 */
	I2C_SendAck(0);
	buf[2] = I2C_ReceiveByte();  /* 温度CRC */
	I2C_SendAck(0);
	buf[3] = I2C_ReceiveByte();  /* 湿度高字节 */
	I2C_SendAck(0);
	buf[4] = I2C_ReceiveByte();  /* 湿度低字节 */
	I2C_SendAck(0);
	buf[5] = I2C_ReceiveByte();  /* 湿度CRC */
	I2C_SendAck(1);
	I2C_Stop();

	/* CRC校验 */
	if (SHT30_CRC8(buf, 2) != buf[2]) return 5;
	if (SHT30_CRC8(buf + 3, 2) != buf[5]) return 6;

	/* 转换为物理量 */
	raw_temp = ((uint16_t)buf[0] << 8) | buf[1];
	raw_humi = ((uint16_t)buf[3] << 8) | buf[4];

	*temperature = -45.0f + 175.0f * raw_temp / 65535.0f;
	*humidity    = 100.0f * raw_humi / 65535.0f;

	return 0;
}

#include "AT24C02.h"
#include "Delay.h"

#define AT24C02_SCL_PIN         GPIO_Pin_10
#define AT24C02_SDA_PIN         GPIO_Pin_11
#define AT24C02_GPIO            GPIOB
#define AT24C02_GPIO_RCC        RCC_APB2Periph_GPIOB

#define SCL_H()                 GPIO_SetBits(AT24C02_GPIO, AT24C02_SCL_PIN)
#define SCL_L()                 GPIO_ResetBits(AT24C02_GPIO, AT24C02_SCL_PIN)
#define SDA_H()                 GPIO_SetBits(AT24C02_GPIO, AT24C02_SDA_PIN)
#define SDA_L()                 GPIO_ResetBits(AT24C02_GPIO, AT24C02_SDA_PIN)
#define SDA_READ()              GPIO_ReadInputDataBit(AT24C02_GPIO, AT24C02_SDA_PIN)

static uint8_t AT24C02_DevAddr = AT24C02_DEV_ADDR;

static void I2C_Delay(void)
{
	Delay_us(5);
}

static void SDA_Out(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = AT24C02_SDA_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(AT24C02_GPIO, &GPIO_InitStructure);
}

static void SDA_In(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = AT24C02_SDA_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(AT24C02_GPIO, &GPIO_InitStructure);
}

static void I2C_Start(void)
{
	SDA_Out();
	SDA_H();
	SCL_H();
	I2C_Delay();
	SDA_L();
	I2C_Delay();
	SCL_L();
	I2C_Delay();
}

static void I2C_Stop(void)
{
	SDA_Out();
	SCL_L();
	SDA_L();
	I2C_Delay();
	SCL_H();
	I2C_Delay();
	SDA_H();
	I2C_Delay();
}

static uint8_t I2C_WaitAck(void)
{
	uint8_t timeout = 200;

	SDA_In();
	I2C_Delay();
	SCL_H();
	I2C_Delay();
	while (SDA_READ() != Bit_RESET)
	{
		if (--timeout == 0)
		{
			SCL_L();
			SDA_Out();
			return 0;
		}
	}
	SCL_L();
	SDA_Out();
	return 1;
}

static void I2C_Ack(void)
{
	SDA_Out();
	SDA_L();
	I2C_Delay();
	SCL_H();
	I2C_Delay();
	SCL_L();
	I2C_Delay();
}

static void I2C_NAck(void)
{
	SDA_Out();
	SDA_H();
	I2C_Delay();
	SCL_H();
	I2C_Delay();
	SCL_L();
	I2C_Delay();
}

static uint8_t I2C_SendByte(uint8_t byte)
{
	uint8_t i;

	SDA_Out();
	for (i = 0; i < 8; i++)
	{
		if ((byte & 0x80) != 0)
		{
			SDA_H();
		}
		else
		{
			SDA_L();
		}
		byte <<= 1;
		I2C_Delay();
		SCL_H();
		I2C_Delay();
		SCL_L();
		I2C_Delay();
	}
	return I2C_WaitAck();
}

static uint8_t I2C_ReadByte(uint8_t ack)
{
	uint8_t i;
	uint8_t byte = 0;

	SDA_In();
	for (i = 0; i < 8; i++)
	{
		byte <<= 1;
		SCL_H();
		I2C_Delay();
		if (SDA_READ() != Bit_RESET)
		{
			byte |= 0x01;
		}
		SCL_L();
		I2C_Delay();
	}

	SDA_Out();
	if (ack != 0)
	{
		I2C_Ack();
	}
	else
	{
		I2C_NAck();
	}
	return byte;
}

static uint8_t AT24C02_WaitReady(void)
{
	uint8_t retry;

	for (retry = 0; retry < 50; retry++)
	{
		I2C_Start();
		if (I2C_SendByte(AT24C02_DevAddr) != 0)
		{
			I2C_Stop();
			return 1;
		}
		I2C_Stop();
		Delay_ms(1);
	}
	return 0;
}

static uint8_t AT24C02_ProbeAddr(uint8_t dev_addr)
{
	I2C_Start();
	if (I2C_SendByte(dev_addr) == 0)
	{
		I2C_Stop();
		return 0;
	}
	I2C_Stop();
	return 1;
}

static void AT24C02_DetectAddr(void)
{
	static const uint8_t addr_list[] =
	{
		0xA0, 0xA2, 0xA4, 0xA6, 0xA8, 0xAA, 0xAC, 0xAE
	};
	uint8_t i;

	for (i = 0; i < sizeof(addr_list); i++)
	{
		if (AT24C02_ProbeAddr(addr_list[i]) != 0)
		{
			AT24C02_DevAddr = addr_list[i];
			return;
		}
	}
}

void AT24C02_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(AT24C02_GPIO_RCC, ENABLE);

	GPIO_InitStructure.GPIO_Pin = AT24C02_SCL_PIN | AT24C02_SDA_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(AT24C02_GPIO, &GPIO_InitStructure);

	SCL_H();
	SDA_H();
	Delay_ms(10);
	AT24C02_DetectAddr();
}

uint8_t AT24C02_WriteBytes(uint8_t addr, const uint8_t *buf, uint16_t len)
{
	uint16_t offset = 0;

	if (buf == 0 || len == 0 || (uint16_t)addr + len > AT24C02_CAPACITY)
	{
		return 0;
	}

	while (offset < len)
	{
		uint8_t page_remain = (uint8_t)(AT24C02_PAGE_SIZE - (addr % AT24C02_PAGE_SIZE));
		uint8_t write_len = (uint8_t)((len - offset < page_remain) ? (len - offset) : page_remain);
		uint8_t i;

		I2C_Start();
		if (I2C_SendByte(AT24C02_DevAddr) == 0)
		{
			I2C_Stop();
			return 0;
		}
		if (I2C_SendByte(addr) == 0)
		{
			I2C_Stop();
			return 0;
		}

		for (i = 0; i < write_len; i++)
		{
			if (I2C_SendByte(buf[offset + i]) == 0)
			{
				I2C_Stop();
				return 0;
			}
		}

		I2C_Stop();
		if (AT24C02_WaitReady() == 0)
		{
			return 0;
		}

		addr = (uint8_t)(addr + write_len);
		offset = (uint16_t)(offset + write_len);
	}

	return 1;
}

uint8_t AT24C02_ReadBytes(uint8_t addr, uint8_t *buf, uint16_t len)
{
	uint16_t i;

	if (buf == 0 || len == 0 || (uint16_t)addr + len > AT24C02_CAPACITY)
	{
		return 0;
	}

	I2C_Start();
	if (I2C_SendByte(AT24C02_DevAddr) == 0)
	{
		I2C_Stop();
		return 0;
	}
	if (I2C_SendByte(addr) == 0)
	{
		I2C_Stop();
		return 0;
	}

	I2C_Start();
	if (I2C_SendByte((uint8_t)(AT24C02_DevAddr | 0x01)) == 0)
	{
		I2C_Stop();
		return 0;
	}

	for (i = 0; i < len; i++)
	{
		buf[i] = I2C_ReadByte((uint8_t)((i + 1) < len));
	}

	I2C_Stop();
	return 1;
}

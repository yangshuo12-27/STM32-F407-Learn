#include "stm32f10x.h"
#include "Delay.h"
#include "DHT11.h"
#include "FreeRTOS.h"
#include "task.h"

#define DHT11_RETRY_MAX         3
#define DHT11_ACK_TIMEOUT_US    1000

static uint8_t DHT11_WaitPin(uint8_t level, uint32_t timeout_us)
{
	while (timeout_us-- > 0)
	{
		if (DHT11_DATA_IN() == (level ? Bit_SET : Bit_RESET))
		{
			return 1;
		}
		Delay_us(1);
	}
	return 0;
}

static void DHT11_Mode_In(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = DHT11_Out_Pin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

static void DHT11_Mode_Out(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = DHT11_Out_Pin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void DHT11_Init(void)
{
	RCC_APB2PeriphClockCmd(DHT11_Out_RCC, ENABLE);
	DHT11_Mode_Out();
	GPIO_SetBits(GPIOA, DHT11_Out_Pin);
	Delay_ms(1000);
}

static uint8_t DHT11_ReadByte(void)
{
	uint8_t i;
	uint8_t data = 0;

	for (i = 0; i < 8; i++)
	{
		if (DHT11_WaitPin(1, 200) == 0)
		{
			return 0xFF;
		}
		Delay_us(40);
		if (DHT11_DATA_IN() == Bit_SET)
		{
			data |= (uint8_t)(0x80 >> i);
		}
		if (DHT11_WaitPin(0, 200) == 0)
		{
			return 0xFF;
		}
	}
	return data;
}

static uint8_t DHT11_ReadOnce(DHT11_Data_TypeDef *DHT11_Data)
{
	uint32_t primask;
	uint8_t h1, h2, t1, t2, chk;

	if (DHT11_Data == 0)
	{
		return DHT11_ERROR;
	}

	primask = __get_PRIMASK();
	__disable_irq();

	DHT11_Mode_Out();
	GPIO_ResetBits(GPIOA, DHT11_Out_Pin);
	Delay_ms(20);
	GPIO_SetBits(GPIOA, DHT11_Out_Pin);
	Delay_us(30);

	DHT11_Mode_In();

	if (DHT11_WaitPin(0, DHT11_ACK_TIMEOUT_US) == 0)
	{
		goto fail;
	}
	if (DHT11_WaitPin(1, DHT11_ACK_TIMEOUT_US) == 0)
	{
		goto fail;
	}
	if (DHT11_WaitPin(0, DHT11_ACK_TIMEOUT_US) == 0)
	{
		goto fail;
	}

	h1 = DHT11_ReadByte();
	h2 = DHT11_ReadByte();
	t1 = DHT11_ReadByte();
	t2 = DHT11_ReadByte();
	chk = DHT11_ReadByte();

	DHT11_Mode_Out();
	GPIO_SetBits(GPIOA, DHT11_Out_Pin);

	if (primask == 0)
	{
		__enable_irq();
	}

	if ((h1 == 0xFF) || (h2 == 0xFF) || (t1 == 0xFF) || (t2 == 0xFF) || (chk == 0xFF))
	{
		return DHT11_ERROR;
	}
	if (chk != (uint8_t)(h1 + h2 + t1 + t2))
	{
		return DHT11_ERROR;
	}

	DHT11_Data->humi_int = h1;
	DHT11_Data->humi_deci = h2;
	DHT11_Data->temp_int = t1;
	DHT11_Data->temp_deci = t2;
	DHT11_Data->check_sum = chk;
	return DHT11_SUCCESS;

fail:
	DHT11_Mode_Out();
	GPIO_SetBits(GPIOA, DHT11_Out_Pin);
	if (primask == 0)
	{
		__enable_irq();
	}
	return DHT11_ERROR;
}

uint8_t Read_DHT11(DHT11_Data_TypeDef *DHT11_Data)
{
	uint8_t i;
	uint8_t ret;

	if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
	{
		vTaskSuspendAll();
	}

	for (i = 0; i < DHT11_RETRY_MAX; i++)
	{
		ret = DHT11_ReadOnce(DHT11_Data);
		if (ret == DHT11_SUCCESS)
		{
			if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
			{
				xTaskResumeAll();
			}
			return DHT11_SUCCESS;
		}
		Delay_ms(300);
	}

	if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
	{
		xTaskResumeAll();
	}
	return DHT11_ERROR;
}

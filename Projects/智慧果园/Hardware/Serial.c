#include "stm32f10x.h"
#include "Serial.h"
#include <string.h>

#define SERIAL_RX_SIZE  1024

static char Serial_RxBuffer[SERIAL_RX_SIZE];
static volatile uint16_t Serial_RxLen;

void Serial_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_Init(USART2, &USART_InitStructure);

	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	USART_Cmd(USART2, ENABLE);
	Serial_FlushRx();
}

void Serial_SendByte(uint8_t byte)
{
	while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
	USART_SendData(USART2, byte);
}

void Serial_SendString(const char *str)
{
	while (*str != '\0')
	{
		Serial_SendByte((uint8_t)*str++);
	}
}

void Serial_SendBuffer(const char *buf, uint16_t len)
{
	uint16_t i;

	for (i = 0; i < len; i++)
	{
		Serial_SendByte((uint8_t)buf[i]);
	}
}

void Serial_FlushRx(void)
{
	Serial_RxLen = 0;
	Serial_RxBuffer[0] = '\0';
}

uint16_t Serial_GetRxLen(void)
{
	return Serial_RxLen;
}

char *Serial_GetRxBuffer(void)
{
	Serial_RxBuffer[Serial_RxLen] = '\0';
	return Serial_RxBuffer;
}

void USART2_IRQHandler(void)
{
	uint8_t data;

	if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
	{
		data = (uint8_t)USART_ReceiveData(USART2);
		if (Serial_RxLen < SERIAL_RX_SIZE - 1)
		{
			Serial_RxBuffer[Serial_RxLen++] = (char)data;
		}
	}
}

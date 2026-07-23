#include "stm32f10x.h"                  // Device header

/**
  * 函    数：继电器初始化
  * 参    数：无
  * 返 回 值：无
  */
void Relay_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_ResetBits(GPIOA, GPIO_Pin_9 | GPIO_Pin_11);
}

void Relay1_ON(void)
{
	GPIO_SetBits(GPIOA, GPIO_Pin_11);
}

void Relay1_OFF(void)
{
	GPIO_ResetBits(GPIOA, GPIO_Pin_11);
}

void Relay1_Turn(void)
{
	if (GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_11) == 0)
	{
		Relay1_ON();
	}
	else
	{
		Relay1_OFF();
	}
}

uint8_t Relay1_GetState(void)
{
	return GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_11) ? 1 : 0;
}

void Relay2_ON(void)
{
	GPIO_SetBits(GPIOA, GPIO_Pin_9);
}

void Relay2_OFF(void)
{
	GPIO_ResetBits(GPIOA, GPIO_Pin_9);
}

void Relay2_Turn(void)
{
	if (GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_9) == 0)
	{
		Relay2_ON();
	}
	else
	{
		Relay2_OFF();
	}
}

uint8_t Relay2_GetState(void)
{
	return GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_9) ? 1 : 0;
}

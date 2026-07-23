#include "stm32f10x.h"
#include "FillLight.h"

void FillLight_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOA, GPIO_Pin_7);
}

void FillLight_ON(void)
{
	GPIO_SetBits(GPIOA, GPIO_Pin_7);
}

void FillLight_OFF(void)
{
	GPIO_ResetBits(GPIOA, GPIO_Pin_7);
}

void FillLight_Set(uint8_t on)
{
	if (on != 0)
	{
		FillLight_ON();
	}
	else
	{
		FillLight_OFF();
	}
}

uint8_t FillLight_GetState(void)
{
	return GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_7) ? 1 : 0;
}

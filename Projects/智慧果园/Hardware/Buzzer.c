#include "stm32f10x.h"                  // Device header
#include "Delay.h"

/**
  * 函    数：蜂鸣器初始化
  * 参    数：无
  * 返 回 值：无
  */
void Buzzer_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_ResetBits(GPIOB, GPIO_Pin_0);
}

void Buzzer_ON(void)
{
	GPIO_SetBits(GPIOB, GPIO_Pin_0);
}

void Buzzer_OFF(void)
{
	GPIO_ResetBits(GPIOB, GPIO_Pin_0);
}

void Buzzer_Beep(uint32_t ms)
{
	Buzzer_ON();
	Delay_ms(ms);
	Buzzer_OFF();
}

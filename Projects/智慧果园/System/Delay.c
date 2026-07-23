#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"

#define DWT_CTRL_REG    (*(volatile uint32_t *)0xE0001000U)
#define DWT_CYCCNT_REG  (*(volatile uint32_t *)0xE0001004U)
#define DWT_CYCCNTENA   (1UL << 0)

static void Delay_us_Systick(uint32_t xus)
{
	SysTick->LOAD = 72 * xus;
	SysTick->VAL = 0x00;
	SysTick->CTRL = 0x00000005;
	while (!(SysTick->CTRL & 0x00010000));
	SysTick->CTRL = 0x00000004;
}

static void Delay_us_DWT(uint32_t xus)
{
	uint32_t start;
	uint32_t ticks;

	if ((CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk) == 0)
	{
		CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
		DWT_CYCCNT_REG = 0;
		DWT_CTRL_REG |= DWT_CYCCNTENA;
	}

	ticks = xus * (SystemCoreClock / 1000000U);
	start = DWT_CYCCNT_REG;
	while ((DWT_CYCCNT_REG - start) < ticks)
	{
	}
}

static void Delay_us_Block(uint32_t xus)
{
	if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
	{
		Delay_us_DWT(xus);
	}
	else
	{
		Delay_us_Systick(xus);
	}
}

void Delay_us(uint32_t xus)
{
	Delay_us_Block(xus);
}

void Delay_ms(uint32_t xms)
{
	if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED && (__get_PRIMASK() == 0U))
	{
		vTaskDelay(pdMS_TO_TICKS(xms));
	}
	else
	{
		while (xms--)
		{
			Delay_us_Block(1000);
		}
	}
}

void Delay_s(uint32_t xs)
{
	while (xs--)
	{
		Delay_ms(1000);
	}
}

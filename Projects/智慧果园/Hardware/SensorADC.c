#include "stm32f10x.h"
#include "SensorADC.h"

#define ADC_SAMPLE_TIMES        16
#define ADC_DISCARD_TIMES       2
#define ADC_TRIM_EACH_SIDE      4

static uint16_t SensorADC_Light;
static uint16_t SensorADC_MQ2;
static uint16_t SensorADC_Soil;

static void SensorADC_ConfigAnalogPins(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

static uint16_t SensorADC_ReadChannelRaw(uint8_t channel)
{
	uint16_t samples[ADC_SAMPLE_TIMES];
	uint8_t i;
	uint8_t j;
	uint16_t temp;
	uint32_t sum;

	ADC_RegularChannelConfig(ADC1, channel, 1, ADC_SampleTime_239Cycles5);

	for (i = 0; i < ADC_DISCARD_TIMES; i++)
	{
		ADC_SoftwareStartConvCmd(ADC1, ENABLE);
		while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
		(void)ADC_GetConversionValue(ADC1);
	}

	for (i = 0; i < ADC_SAMPLE_TIMES; i++)
	{
		ADC_SoftwareStartConvCmd(ADC1, ENABLE);
		while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
		samples[i] = (uint16_t)ADC_GetConversionValue(ADC1);
	}

	for (i = 0; i < ADC_SAMPLE_TIMES - 1; i++)
	{
		for (j = 0; j < ADC_SAMPLE_TIMES - 1 - i; j++)
		{
			if (samples[j] > samples[j + 1])
			{
				temp = samples[j];
				samples[j] = samples[j + 1];
				samples[j + 1] = temp;
			}
		}
	}

	sum = 0;
	for (i = ADC_TRIM_EACH_SIDE; i < ADC_SAMPLE_TIMES - ADC_TRIM_EACH_SIDE; i++)
	{
		sum += samples[i];
	}

	return (uint16_t)(sum / (ADC_SAMPLE_TIMES - 2 * ADC_TRIM_EACH_SIDE));
}

void SensorADC_Init(void)
{
	ADC_InitTypeDef ADC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOA, ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);

	SensorADC_ConfigAnalogPins();

	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_Cmd(ADC1, ENABLE);

	ADC_ResetCalibration(ADC1);
	while (ADC_GetResetCalibrationStatus(ADC1) == SET);
	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1) == SET);

	SensorADC_Update();
}

void SensorADC_Update(void)
{
	SensorADC_ConfigAnalogPins();

	SensorADC_Light = SensorADC_ReadChannelRaw(ADC_Channel_4);
	SensorADC_MQ2 = SensorADC_ReadChannelRaw(ADC_Channel_5);
	SensorADC_Soil = SensorADC_ReadChannelRaw(ADC_Channel_6);
}

uint16_t SensorADC_GetLight(void)
{
	return SensorADC_Light;
}

uint16_t SensorADC_GetMQ2(void)
{
	return SensorADC_MQ2;
}

uint16_t SensorADC_GetSoil(void)
{
	return SensorADC_Soil;
}

uint16_t SensorADC_ToVolt_x10(uint16_t raw)
{
	return (uint16_t)((uint32_t)raw * 50u / 4095u);
}

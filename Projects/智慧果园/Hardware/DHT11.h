#ifndef __DHT11_H
#define __DHT11_H

#define DHT11_Out_Pin		GPIO_Pin_1
#define DHT11_Out_RCC		RCC_APB2Periph_GPIOA
#define DHT11_DATA_IN()		GPIO_ReadInputDataBit(GPIOA, DHT11_Out_Pin)

typedef struct
{
	uint8_t humi_int;
	uint8_t humi_deci;
	uint8_t temp_int;
	uint8_t temp_deci;
	uint8_t check_sum;
} DHT11_Data_TypeDef;

#define DHT11_SUCCESS		1
#define DHT11_ERROR			0

void DHT11_Init(void);
uint8_t Read_DHT11(DHT11_Data_TypeDef *DHT11_Data);

#endif

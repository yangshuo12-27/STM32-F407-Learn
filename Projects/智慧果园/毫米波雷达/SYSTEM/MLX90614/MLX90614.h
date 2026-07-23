#ifndef _MLX90614_H
#define _MLX90614_H
#include "stm32f10x.h"
#include "sys.h"
#include "delay.h"
#include "usart.h"

/*
(模块)VCC---3.3(开发板)
(模块)GND---GND(开发板)
(模块)SDA---PA7(开发板)
(模块)SCL---PA6(开发板)
*/


#define IIC_SDA_OUTMODE() {GPIOA->CRL&=0x0FFFFFFF;GPIOA->CRL|=0x30000000;}
#define IIC_SDA_INPUTMODE() {GPIOA->CRL&=0x0FFFFFFF;GPIOA->CRL|=0x80000000;}
#define IIC_SDA_OUT PAout(7)  //数据线输出
#define IIC_SDA_IN PAin(7)  //数据线输入
#define IIC_SCL PAout(6)  //时钟线

void IIC_Init(void);
void IIC_Start(void);
void IIC_Stop(void);
u8 IIC_GetACK(void);
void IIC_SendACK(u8 stat);
void IIC_WriteOneByteData(u8 data);
u8 IIC_ReadOneByteData(void);

void MLX90614_Init(void);

//读取温度数据
float read_MLX90614(void);
float calculate_min_average(void);
#endif



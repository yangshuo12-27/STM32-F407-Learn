#ifndef _KEY_H
#define _KEY_H
#include "stm32f10x.h"
#include "sys.h"
#include "delay.h"

//定义按键IO口 硬件连接:PE3 PE4
#define KEY0 PAin(0)
#define KEY1 PCin(5)
#define KEY2 PAin(15) 

//函数声明
void KEY_Init(void);
u8 KEY_Scan(void);
#endif

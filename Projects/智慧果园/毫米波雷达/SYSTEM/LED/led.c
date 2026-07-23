#include "led.h"
/*
函数功能: LED初始化
硬件连接: PA8 PD2
特性: 低电平点亮
*/
void LED_Init(void)
{
    //开时钟
    RCC->APB2ENR|=1<<2;
    RCC->APB2ENR|=1<<5;
    
    //配置GPIO口
    GPIOA->CRH&=0xFFFFFFF0;
    GPIOA->CRH|=0x00000003;
    GPIOD->CRL&=0xFFFFFF0FF;
    GPIOD->CRL|=0x000000300;
    
    //上拉
    GPIOA->ODR|=1<<8;
    GPIOD->ODR|=1<<2;
    
    
}


/*
函数功能: 蜂鸣器初始化
硬件连接: PC7
特性: 高电平响
*/
void BEEP_Init(void)
{
   RCC->APB2ENR|=1<<4;
   GPIOC->CRL&=0x0FFFFFFF;
   GPIOC->CRL|=0x30000000;
   BEEP=0;
}




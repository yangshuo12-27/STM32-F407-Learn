/*
# Copyright (c) 2024 DS小龙哥
#
# Author: DS小龙哥
# WeChat Official Account: DS小龙哥嵌入式技术资讯
# Bilibili: https://space.bilibili.com/68130189
# CSDN Blog: https://blog.csdn.net/xiaolong1126626497
#
# This file is part of <DS小龙哥的项目>.
# <DS小龙哥的项目> is free software: you can redistribute it and/or modify
# it under the terms of the MIT License as published by
# the Massachusetts Institute of Technology.
#
# <DS小龙哥的项目> is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# MIT License for more details.
#
# You should have received a copy of the MIT License
# along with <DS小龙哥的项目>. If not, see <https://opensource.org/licenses/MIT>.
*/

#include "sys.h"
/*
函数功能: 设置中断优先级
*/
void STM32_SetPriority(IRQn_Type IRQn, uint32_t PreemptPriority, uint32_t SubPriority)
{
	uint32_t EncodePriority;
	NVIC_SetPriorityGrouping(NVIC_PriorityGroup_2);//设置优先级分组
	EncodePriority = NVIC_EncodePriority(NVIC_PriorityGroup_2,PreemptPriority, SubPriority);//获取优先级编码
	NVIC_SetPriority(IRQn, EncodePriority);//设置优先级
	NVIC_EnableIRQ(IRQn);//使能中断线
}

/*
函数功能: 滴答定时器初始化函数
*/
void SysTickInit(void)
{
    SysTick->CTRL&=~(1<<2); //选择外部时钟源
    SysTick->LOAD=500*9000; //500ms中断一次
    SysTick->VAL=0;         //计数器的值清成0
    SysTick->CTRL|=1<<1;    //开启中断
    SysTick->CTRL|=1<<0;    //开启滴答定时器
}

/*
函数功能: 编写滴答定时器中断服务函数
*/
void SysTick_Handler(void)
{
    
}



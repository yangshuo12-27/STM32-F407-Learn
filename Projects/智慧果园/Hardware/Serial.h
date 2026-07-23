#ifndef __SERIAL_H
#define __SERIAL_H

#include "stm32f10x.h"

void Serial_Init(void);
void Serial_SendByte(uint8_t byte);
void Serial_SendString(const char *str);
void Serial_SendBuffer(const char *buf, uint16_t len);
void Serial_FlushRx(void);
uint16_t Serial_GetRxLen(void);
char *Serial_GetRxBuffer(void);

#endif

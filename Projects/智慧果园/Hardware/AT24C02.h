#ifndef __AT24C02_H
#define __AT24C02_H

#include "stm32f10x.h"

#define AT24C02_DEV_ADDR        0xA0
#define AT24C02_PAGE_SIZE       8
#define AT24C02_CAPACITY        256
#define AT24C02_WRITE_DELAY_MS  5

void AT24C02_Init(void);
uint8_t AT24C02_WriteBytes(uint8_t addr, const uint8_t *buf, uint16_t len);
uint8_t AT24C02_ReadBytes(uint8_t addr, uint8_t *buf, uint16_t len);

#endif

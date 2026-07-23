#ifndef __SENSORADC_H
#define __SENSORADC_H

void SensorADC_Init(void);
void SensorADC_Update(void);
uint16_t SensorADC_GetLight(void);
uint16_t SensorADC_GetMQ2(void);
uint16_t SensorADC_GetSoil(void);

uint16_t SensorADC_ToVolt_x10(uint16_t raw);

#endif

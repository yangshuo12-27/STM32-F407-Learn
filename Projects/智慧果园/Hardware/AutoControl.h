#ifndef __AUTOCONTROL_H
#define __AUTOCONTROL_H

#include "stm32f10x.h"

typedef enum
{
	RUN_MODE_AUTO = 0,
	RUN_MODE_MANUAL
} RunMode_t;

typedef struct
{
	uint8_t dht_ok;
	uint8_t temp_int;
	uint8_t temp_deci;
	uint8_t humi_int;
	uint16_t light_raw;
	uint16_t soil_raw;
	uint16_t mq2_raw;
} SensorSnapshot_t;

typedef struct
{
	uint8_t temp_open_c;
	uint8_t temp_close_c;
	uint8_t light_on_x10;
	uint8_t light_off_x10;
	uint8_t soil_on_x10;
	uint8_t soil_off_x10;
	uint8_t mq2_on_x10;
	uint8_t mq2_off_x10;
} AutoThreshold_t;

#define ALARM_MQ2        0x01
#define ALARM_SOIL_DRY   0x02
#define ALARM_LIGHT_LOW  0x04
#define ALARM_TEMP_HIGH  0x08
#define ALARM_TEMP_LOW   0x10

void AutoControl_Init(void);
void AutoControl_SetMode(RunMode_t mode);
RunMode_t AutoControl_GetMode(void);
void AutoControl_ToggleMode(void);
void AutoControl_UpdateAuto(const SensorSnapshot_t *sensor);
void AutoControl_ApplyManual(int8_t pump, int8_t fan, int8_t fill, int16_t servo);
void AutoControl_ApplyCommand(int8_t mode_set, int8_t pump, int8_t fan, int8_t fill, int16_t servo);
uint8_t AutoControl_GetAlarmFlags(void);
uint8_t AutoControl_GetPumpState(void);
uint8_t AutoControl_GetFanState(void);
uint8_t AutoControl_GetFillState(void);
uint8_t AutoControl_GetServoAngle(void);
const char *AutoControl_GetServoActionText(void);
AutoThreshold_t *AutoControl_GetThreshold(void);
void AutoControl_AdjustThreshold(uint8_t item, int8_t delta);
void AutoControl_SetThreshold(const AutoThreshold_t *threshold);

#endif

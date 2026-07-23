#include "AutoControl.h"
#include "Relay.h"
#include "FillLight.h"
#include "Servo.h"
#include "SensorADC.h"
#include "AT24C02.h"
#include <string.h>

#define THRESHOLD_EEPROM_MAGIC  0xA5
#define THRESHOLD_EEPROM_ADDR   0x00

typedef struct
{
	uint8_t magic;
	AutoThreshold_t data;
	uint8_t checksum;
} ThresholdRecord_t;

static RunMode_t Run_Mode = RUN_MODE_AUTO;
static AutoThreshold_t Threshold;
static uint8_t Alarm_Flags;
static uint8_t Pump_State;
static uint8_t Fan_State;
static uint8_t Fill_State;
static uint8_t Servo_Angle;
static char Servo_Action[12];

static void AutoControl_SetDefaultThreshold(void)
{
	Threshold.temp_open_c = 30;
	Threshold.temp_close_c = 20;
	Threshold.light_on_x10 = 30;
	Threshold.light_off_x10 = 20;
	Threshold.soil_on_x10 = 20;
	Threshold.soil_off_x10 = 15;
	Threshold.mq2_on_x10 = 20;
	Threshold.mq2_off_x10 = 15;
}

static uint8_t AutoControl_ThresholdChecksum(const AutoThreshold_t *threshold)
{
	const uint8_t *bytes = (const uint8_t *)threshold;
	uint8_t i;
	uint8_t sum = 0;

	for (i = 0; i < sizeof(AutoThreshold_t); i++)
	{
		sum ^= bytes[i];
	}
	return sum;
}

static void AutoControl_ClampThreshold(void)
{
	if (Threshold.temp_open_c > 50)
	{
		Threshold.temp_open_c = 50;
	}
	if (Threshold.temp_close_c >= Threshold.temp_open_c)
	{
		Threshold.temp_close_c = (uint8_t)(Threshold.temp_open_c - 1);
	}

	if (Threshold.light_on_x10 > 50)
	{
		Threshold.light_on_x10 = 50;
	}
	if (Threshold.light_off_x10 >= Threshold.light_on_x10)
	{
		Threshold.light_off_x10 = (uint8_t)(Threshold.light_on_x10 - 1);
	}

	if (Threshold.soil_on_x10 > 50)
	{
		Threshold.soil_on_x10 = 50;
	}
	if (Threshold.soil_off_x10 > Threshold.soil_on_x10)
	{
		Threshold.soil_off_x10 = Threshold.soil_on_x10;
	}

	if (Threshold.mq2_on_x10 > 50)
	{
		Threshold.mq2_on_x10 = 50;
	}
	if (Threshold.mq2_off_x10 > Threshold.mq2_on_x10)
	{
		Threshold.mq2_off_x10 = Threshold.mq2_on_x10;
	}
}

static void AutoControl_SaveThreshold(void)
{
	ThresholdRecord_t record;
	ThresholdRecord_t verify;
	uint8_t retry;

	record.magic = THRESHOLD_EEPROM_MAGIC;
	record.data = Threshold;
	record.checksum = AutoControl_ThresholdChecksum(&Threshold);

	for (retry = 0; retry < 3; retry++)
	{
		if (AT24C02_WriteBytes(THRESHOLD_EEPROM_ADDR, (const uint8_t *)&record, sizeof(record)) == 0)
		{
			continue;
		}
		if (AT24C02_ReadBytes(THRESHOLD_EEPROM_ADDR, (uint8_t *)&verify, sizeof(verify)) == 0)
		{
			continue;
		}
		if (memcmp(&record, &verify, sizeof(record)) == 0)
		{
			return;
		}
	}
}

static void AutoControl_LoadThreshold(void)
{
	ThresholdRecord_t record;

	if (AT24C02_ReadBytes(THRESHOLD_EEPROM_ADDR, (uint8_t *)&record, sizeof(record)) == 0)
	{
		return;
	}
	if (record.magic != THRESHOLD_EEPROM_MAGIC)
	{
		return;
	}
	if (record.checksum != AutoControl_ThresholdChecksum(&record.data))
	{
		return;
	}

	Threshold = record.data;
	AutoControl_ClampThreshold();
}

static void AutoControl_ApplyActuators(void)
{
	if (Pump_State != 0)
	{
		Relay1_ON();
	}
	else
	{
		Relay1_OFF();
	}

	if (Fan_State != 0)
	{
		Relay2_ON();
	}
	else
	{
		Relay2_OFF();
	}

	FillLight_Set(Fill_State);
	Servo_SetAngle(Servo_Angle);
}

void AutoControl_Init(void)
{
	Run_Mode = RUN_MODE_AUTO;
	AutoControl_SetDefaultThreshold();
	AutoControl_LoadThreshold();
	Alarm_Flags = 0;
	Pump_State = 0;
	Fan_State = 0;
	Fill_State = 0;
	Servo_Angle = 0;
	Servo_Action[0] = 0;
	AutoControl_ApplyActuators();
}

void AutoControl_SetMode(RunMode_t mode)
{
	Run_Mode = mode;
}

RunMode_t AutoControl_GetMode(void)
{
	return Run_Mode;
}

void AutoControl_ToggleMode(void)
{
	Run_Mode = (Run_Mode == RUN_MODE_AUTO) ? RUN_MODE_MANUAL : RUN_MODE_AUTO;
}

AutoThreshold_t *AutoControl_GetThreshold(void)
{
	return &Threshold;
}

void AutoControl_AdjustThreshold(uint8_t item, int8_t delta)
{
	switch (item)
	{
	case 0:
		Threshold.temp_open_c = (uint8_t)((int16_t)Threshold.temp_open_c + delta);
		if (Threshold.temp_open_c > 50) Threshold.temp_open_c = 50;
		if (Threshold.temp_open_c < Threshold.temp_close_c + 1) Threshold.temp_open_c = Threshold.temp_close_c + 1;
		break;
	case 1:
		Threshold.temp_close_c = (uint8_t)((int16_t)Threshold.temp_close_c + delta);
		if (Threshold.temp_close_c > Threshold.temp_open_c - 1) Threshold.temp_close_c = Threshold.temp_open_c - 1;
		if (Threshold.temp_close_c < 0) Threshold.temp_close_c = 0;
		break;
	case 2:
		Threshold.light_on_x10 = (uint8_t)((int16_t)Threshold.light_on_x10 + delta);
		if (Threshold.light_on_x10 > 50) Threshold.light_on_x10 = 50;
		if (Threshold.light_on_x10 <= Threshold.light_off_x10) Threshold.light_on_x10 = (uint8_t)(Threshold.light_off_x10 + 1);
		break;
	case 3:
		Threshold.light_off_x10 = (uint8_t)((int16_t)Threshold.light_off_x10 + delta);
		if (Threshold.light_off_x10 >= Threshold.light_on_x10) Threshold.light_off_x10 = (uint8_t)(Threshold.light_on_x10 - 1);
		if (Threshold.light_off_x10 > 50) Threshold.light_off_x10 = 50;
		break;
	case 4:
		Threshold.soil_on_x10 = (uint8_t)((int16_t)Threshold.soil_on_x10 + delta);
		if (Threshold.soil_on_x10 > 50) Threshold.soil_on_x10 = 50;
		break;
	case 5:
		Threshold.soil_off_x10 = (uint8_t)((int16_t)Threshold.soil_off_x10 + delta);
		if (Threshold.soil_off_x10 > Threshold.soil_on_x10) Threshold.soil_off_x10 = Threshold.soil_on_x10;
		break;
	default:
		break;
	}

	AutoControl_SaveThreshold();
}

void AutoControl_SetThreshold(const AutoThreshold_t *threshold)
{
	if (threshold == 0)
	{
		return;
	}

	Threshold = *threshold;
	AutoControl_ClampThreshold();
	AutoControl_SaveThreshold();
}

const char *AutoControl_GetServoActionText(void)
{
	if (Servo_Angle >= 170)
	{
		return "AutoOpen";
	}
	if (Servo_Angle <= 10)
	{
		return "AutoClose";
	}
	return "AutoHold";
}

void AutoControl_UpdateAuto(const SensorSnapshot_t *sensor)
{
	uint16_t light_v;
	uint16_t soil_v;
	uint16_t mq2_v;
	uint16_t temp_x10;
	uint8_t gas_alarm;

	if (sensor == 0 || Run_Mode != RUN_MODE_AUTO)
	{
		return;
	}

	Alarm_Flags = 0;
	light_v = SensorADC_ToVolt_x10(sensor->light_raw);
	soil_v = SensorADC_ToVolt_x10(sensor->soil_raw);
	mq2_v = SensorADC_ToVolt_x10(sensor->mq2_raw);
	gas_alarm = (mq2_v > Threshold.mq2_on_x10) ? 1 : 0;

	if (light_v > Threshold.light_on_x10)
	{
		Fill_State = 1;
	}
	else if (light_v < Threshold.light_off_x10)
	{
		Fill_State = 0;
	}

	if (soil_v > Threshold.soil_on_x10)
	{
		Pump_State = 1;
		Alarm_Flags |= ALARM_SOIL_DRY;
	}
	else if (soil_v < Threshold.soil_off_x10)
	{
		Pump_State = 0;
	}

	if (gas_alarm != 0)
	{
		Fan_State = 1;
		Servo_Angle = 180;
		Alarm_Flags |= ALARM_MQ2;
	}
	else if (mq2_v < Threshold.mq2_off_x10)
	{
		Fan_State = 0;
	}

	if (gas_alarm != 0 || (Fan_State != 0 && mq2_v >= Threshold.mq2_off_x10))
	{
		Servo_Angle = 180;
	}
	else if (sensor->dht_ok != 0)
	{
		temp_x10 = (uint16_t)(sensor->temp_int * 10u + sensor->temp_deci);
		if (temp_x10 <= (uint16_t)(Threshold.temp_close_c * 10))
		{
			Servo_Angle = 0;
			Alarm_Flags |= ALARM_TEMP_LOW;
		}
		else if (temp_x10 >= (uint16_t)(Threshold.temp_open_c * 10))
		{
			Servo_Angle = 180;
			Alarm_Flags |= ALARM_TEMP_HIGH;
		}
	}

	AutoControl_ApplyActuators();
}

void AutoControl_ApplyManual(int8_t pump, int8_t fan, int8_t fill, int16_t servo)
{
	if (Run_Mode != RUN_MODE_MANUAL)
	{
		return;
	}

	if (pump >= 0)
	{
		Pump_State = (uint8_t)pump;
	}
	if (fan >= 0)
	{
		Fan_State = (uint8_t)fan;
	}
	if (fill >= 0)
	{
		Fill_State = (uint8_t)fill;
	}
	if (servo >= 0)
	{
		if (servo > 180)
		{
			servo = 180;
		}
		Servo_Angle = (uint8_t)servo;
	}

	AutoControl_ApplyActuators();
}

void AutoControl_ApplyCommand(int8_t mode_set, int8_t pump, int8_t fan, int8_t fill, int16_t servo)
{
	if (mode_set >= 0)
	{
		Run_Mode = (RunMode_t)mode_set;
	}

	if (Run_Mode != RUN_MODE_MANUAL)
	{
		return;
	}

	AutoControl_ApplyManual(pump, fan, fill, servo);
}

uint8_t AutoControl_GetAlarmFlags(void)
{
	return Alarm_Flags;
}

uint8_t AutoControl_GetPumpState(void)
{
	return Pump_State;
}

uint8_t AutoControl_GetFanState(void)
{
	return Fan_State;
}

uint8_t AutoControl_GetFillState(void)
{
	return Fill_State;
}

uint8_t AutoControl_GetServoAngle(void)
{
	return Servo_Angle;
}

#include "stm32f10x.h"
#include "ESP8266.h"
#include "Serial.h"
#include "WiFiConfig.h"
#include "Delay.h"
#include "AutoControl.h"
#include "SensorADC.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static ESP_Status_t ESP_Status = ESP_WIFI_IDLE;
static uint8_t ESP_LinkConnected;
static volatile uint8_t ESP_Busy;
static char ESP_PublishJson[320];
static uint8_t ESP_InPassthrough;
static char ESP_CmdLineBuf[256];
static uint16_t ESP_CmdLineLen;

static uint8_t ESP_WaitResponse(const char *token, uint32_t timeout_ms);
static uint8_t ESP_SendCmdWait(const char *cmd, const char *token, uint32_t timeout_ms);
static void ESP8266_HandleCommand(const char *line);
static void ESP8266_FeedRxByte(char byte);

static uint8_t ESP_CheckCmdError(uint16_t base_len)
{
	char *rx = Serial_GetRxBuffer() + base_len;

	if (strstr(rx, "\r\nERROR") != 0 || strstr(rx, "ERROR:") != 0)
	{
		return 1;
	}
	return 0;
}

static uint8_t ESP_WaitResponse(const char *token, uint32_t timeout_ms)
{
	uint32_t elapsed = 0;
	uint16_t base_len = Serial_GetRxLen();

	while (elapsed < timeout_ms)
	{
		Delay_ms(20);
		elapsed += 20;
		if (Serial_GetRxLen() > base_len)
		{
			if (ESP_InPassthrough == 0 && ESP_CheckCmdError(base_len) != 0)
			{
				return 0;
			}
			if (token != 0 && strstr(Serial_GetRxBuffer() + base_len, token) != 0)
			{
				return 1;
			}
		}
	}
	return 0;
}

static uint8_t ESP_SendCmdWait(const char *cmd, const char *token, uint32_t timeout_ms)
{
	uint8_t retry;

	for (retry = 0; retry < 5; retry++)
	{
		Serial_FlushRx();
		Serial_SendString(cmd);
		if (ESP_WaitResponse(token, timeout_ms) != 0)
		{
			return 1;
		}
	}
	return 0;
}

static void ESP8266_ExitPassthrough(void)
{
	Serial_SendString("+++");
	Delay_ms(100);
	Serial_SendString("+++");
	Delay_ms(100);
	Serial_FlushRx();
	ESP_InPassthrough = 0;
}

static void ESP8266_CloseTcpLink(void)
{
	if (ESP_InPassthrough != 0)
	{
		ESP8266_ExitPassthrough();
	}

	(void)ESP_SendCmdWait("AT+CIPCLOSE\r\n", "OK", 3000);
	Delay_ms(300);
	Serial_FlushRx();
}

void ESP8266_Init(void)
{
	ESP_Status = ESP_WIFI_IDLE;
	ESP_LinkConnected = 0;
	ESP_Busy = 0;
	ESP_InPassthrough = 0;

	ESP8266_ExitPassthrough();
	(void)ESP_SendCmdWait("AT\r\n", "OK", 2000);
}

uint8_t ESP8266_IsBusy(void)
{
	return ESP_Busy;
}

uint8_t ESP8266_IsConnected(void)
{
	return ESP_LinkConnected;
}

void ESP8266_ForceDisconnect(void)
{
	ESP_LinkConnected = 0;
	ESP_Status = ESP_MQTT_FAIL;
	ESP_CmdLineLen = 0;
	ESP_CmdLineBuf[0] = '\0';
	if (ESP_Busy == 0)
	{
		ESP8266_CloseTcpLink();
	}
	else
	{
		ESP_InPassthrough = 0;
	}
}

static int8_t ESP_JsonGetInt8(const char *json, const char *key, int8_t default_val)
{
	char tag[24];
	const char *pos;
	int value;

	sprintf(tag, "\"%s\":", key);
	pos = strstr(json, tag);
	if (pos == 0)
	{
		return default_val;
	}
	pos += strlen(tag);
	while (*pos == ' ')
	{
		pos++;
	}
	value = atoi(pos);
	return (int8_t)value;
}

static int16_t ESP_JsonGetInt16(const char *json, const char *key, int16_t default_val)
{
	char tag[24];
	const char *pos;

	sprintf(tag, "\"%s\":", key);
	pos = strstr(json, tag);
	if (pos == 0)
	{
		return default_val;
	}
	pos += strlen(tag);
	while (*pos == ' ')
	{
		pos++;
	}
	return (int16_t)atoi(pos);
}

static uint8_t ESP_JsonGetUInt8(const char *json, const char *key, uint8_t default_val)
{
	int16_t value;

	value = ESP_JsonGetInt16(json, key, (int16_t)default_val);
	if (value < 0)
	{
		value = 0;
	}
	if (value > 255)
	{
		value = 255;
	}
	return (uint8_t)value;
}

static void ESP8266_ApplyThresholdJson(const char *json)
{
	AutoThreshold_t threshold;
	AutoThreshold_t *current;

	current = AutoControl_GetThreshold();
	threshold = *current;

	threshold.temp_open_c = ESP_JsonGetUInt8(json, "temp_open", threshold.temp_open_c);
	threshold.temp_close_c = ESP_JsonGetUInt8(json, "temp_close", threshold.temp_close_c);
	threshold.light_on_x10 = ESP_JsonGetUInt8(json, "light_on_x10", threshold.light_on_x10);
	threshold.light_off_x10 = ESP_JsonGetUInt8(json, "light_off_x10", threshold.light_off_x10);
	threshold.soil_on_x10 = ESP_JsonGetUInt8(json, "soil_on_x10", threshold.soil_on_x10);
	threshold.soil_off_x10 = ESP_JsonGetUInt8(json, "soil_off_x10", threshold.soil_off_x10);
	threshold.mq2_on_x10 = ESP_JsonGetUInt8(json, "mq2_on_x10", threshold.mq2_on_x10);
	threshold.mq2_off_x10 = ESP_JsonGetUInt8(json, "mq2_off_x10", threshold.mq2_off_x10);
	AutoControl_SetThreshold(&threshold);
}

static void ESP8266_HandleCommand(const char *line)
{
	if (line == 0 || line[0] != '{' || strstr(line, "\"cmd\"") == 0)
	{
		return;
	}

	if (strstr(line, "\"control\"") != 0)
	{
		AutoControl_ApplyCommand(
			ESP_JsonGetInt8(line, "mode", -1),
			ESP_JsonGetInt8(line, "pump", -1),
			ESP_JsonGetInt8(line, "fan", -1),
			ESP_JsonGetInt8(line, "fill", -1),
			ESP_JsonGetInt16(line, "servo", -1));
		return;
	}

	if (strstr(line, "\"threshold\"") != 0)
	{
		ESP8266_ApplyThresholdJson(line);
	}
}

static void ESP8266_FeedRxByte(char byte)
{
	if (byte == '\n' || byte == '\r')
	{
		if (ESP_CmdLineLen > 0)
		{
			ESP_CmdLineBuf[ESP_CmdLineLen] = '\0';
			ESP8266_HandleCommand(ESP_CmdLineBuf);
			ESP_CmdLineLen = 0;
		}
		return;
	}

	if (ESP_CmdLineLen < sizeof(ESP_CmdLineBuf) - 1)
	{
		ESP_CmdLineBuf[ESP_CmdLineLen++] = byte;
	}
}

void ESP8266_Process(void)
{
	uint16_t len;
	uint16_t i;
	char *rx;

	if (ESP_Busy != 0 || ESP_LinkConnected == 0)
	{
		return;
	}

	len = Serial_GetRxLen();
	if (len == 0)
	{
		return;
	}

	rx = Serial_GetRxBuffer();
	for (i = 0; i < len; i++)
	{
		ESP8266_FeedRxByte(rx[i]);
	}
	Serial_FlushRx();
}

ESP_Status_t ESP8266_GetStatus(void)
{
	return ESP_Status;
}

const char *ESP8266_GetStatusText(void)
{
	switch (ESP_Status)
	{
	case ESP_WIFI_CONNECTING: return "WiFi...";
	case ESP_WIFI_OK:         return "WiFi OK";
	case ESP_WIFI_FAIL:       return "WiFi ERR";
	case ESP_MQTT_OK:         return "TCP OK";
	case ESP_MQTT_FAIL:       return "TCP ERR";
	default:                  return "Net Idle";
	}
}

uint8_t ESP8266_TCP_Connect(void)
{
	char cmd[128];
	uint8_t retry;

	ESP_Busy = 1;
	ESP_Status = ESP_WIFI_CONNECTING;
	ESP_LinkConnected = 0;
	ESP_InPassthrough = 0;

	if (ESP_SendCmdWait("AT\r\n", "OK", 2000) == 0)
	{
		ESP_Status = ESP_WIFI_FAIL;
		goto fail;
	}

	if (ESP_SendCmdWait("AT+CWMODE=1\r\n", "OK", 2000) == 0)
	{
		ESP_Status = ESP_WIFI_FAIL;
		goto fail;
	}

	(void)ESP_SendCmdWait("AT+RST\r\n", "OK", 1000);
	Delay_ms(1000);
	Delay_ms(1000);
	Delay_ms(1000);

	sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"\r\n", WIFI_SSID, WIFI_PASSWORD);
	for (retry = 0; retry < 20; retry++)
	{
		if (ESP_SendCmdWait(cmd, "WIFI GOT IP", 5000) != 0)
		{
			break;
		}
	}
	if (retry >= 20)
	{
		ESP_Status = ESP_WIFI_FAIL;
		goto fail;
	}
	ESP_Status = ESP_WIFI_OK;

	if (ESP_SendCmdWait("AT+CIPMUX=0\r\n", "OK", 3000) == 0)
	{
		ESP_Status = ESP_MQTT_FAIL;
		goto fail;
	}

	sprintf(cmd, "AT+CIPSTART=\"TCP\",\"%s\",%d\r\n", TCP_SERVER_HOST, TCP_SERVER_PORT);
	for (retry = 0; retry < 3; retry++)
	{
		if (ESP_SendCmdWait(cmd, "OK", 10000) != 0)
		{
			break;
		}
		Delay_ms(1000);
	}
	if (retry >= 3)
	{
		ESP_Status = ESP_MQTT_FAIL;
		goto fail;
	}

	if (ESP_SendCmdWait("AT+CIPMODE=1\r\n", "OK", 5000) == 0)
	{
		ESP_Status = ESP_MQTT_FAIL;
		goto fail;
	}

	Serial_FlushRx();
	Serial_SendString("AT+CIPSEND\r\n");
	if (ESP_WaitResponse(">", 10000) == 0)
	{
		ESP_Status = ESP_MQTT_FAIL;
		goto fail;
	}

	Serial_FlushRx();
	ESP_InPassthrough = 1;
	ESP_LinkConnected = 1;
	ESP_Status = ESP_MQTT_OK;
	ESP_Busy = 0;
	return 1;

fail:
	ESP8266_CloseTcpLink();
	ESP_LinkConnected = 0;
	ESP_InPassthrough = 0;
	ESP_Busy = 0;
	return 0;
}

uint8_t ESP8266_TCP_Reconnect(void)
{
	return ESP8266_TCP_Connect();
}

uint8_t ESP8266_PublishSensorData(const SensorSnapshot_t *sensor)
{
	uint16_t len;
	uint16_t light_v;
	uint16_t soil_v;
	uint16_t mq2_v;
	const char *mode_text;
	char *json = ESP_PublishJson;
	AutoThreshold_t *th;

	if (sensor == 0 || ESP_LinkConnected == 0 || ESP_InPassthrough == 0)
	{
		return 0;
	}

	light_v = SensorADC_ToVolt_x10(sensor->light_raw);
	soil_v = SensorADC_ToVolt_x10(sensor->soil_raw);
	mq2_v = SensorADC_ToVolt_x10(sensor->mq2_raw);
	mode_text = (AutoControl_GetMode() == RUN_MODE_AUTO) ? "auto" : "manual";
	th = AutoControl_GetThreshold();

	if (sensor->dht_ok != 0)
	{
		sprintf(json,
			"{\"temp\":%u.%u,\"humi\":%u,\"light\":%u.%u,\"soil\":%u.%u,\"mq2\":%u.%u,"
			"\"mode\":\"%s\",\"pump\":%u,\"fan\":%u,\"fill\":%u,\"servo\":%u,\"alarm\":%u,"
			"\"temp_open\":%u,\"temp_close\":%u,"
			"\"light_on_x10\":%u,\"light_off_x10\":%u,"
			"\"soil_on_x10\":%u,\"soil_off_x10\":%u,"
			"\"mq2_on_x10\":%u,\"mq2_off_x10\":%u}",
			sensor->temp_int, sensor->temp_deci, sensor->humi_int,
			light_v / 10, light_v % 10,
			soil_v / 10, soil_v % 10,
			mq2_v / 10, mq2_v % 10,
			mode_text,
			AutoControl_GetPumpState(),
			AutoControl_GetFanState(),
			AutoControl_GetFillState(),
			AutoControl_GetServoAngle(),
			AutoControl_GetAlarmFlags(),
			th->temp_open_c, th->temp_close_c,
			th->light_on_x10, th->light_off_x10,
			th->soil_on_x10, th->soil_off_x10,
			th->mq2_on_x10, th->mq2_off_x10);
	}
	else
	{
		sprintf(json,
			"{\"light\":%u.%u,\"soil\":%u.%u,\"mq2\":%u.%u,"
			"\"mode\":\"%s\",\"pump\":%u,\"fan\":%u,\"fill\":%u,\"servo\":%u,\"alarm\":%u,"
			"\"temp_open\":%u,\"temp_close\":%u,"
			"\"light_on_x10\":%u,\"light_off_x10\":%u,"
			"\"soil_on_x10\":%u,\"soil_off_x10\":%u,"
			"\"mq2_on_x10\":%u,\"mq2_off_x10\":%u}",
			light_v / 10, light_v % 10,
			soil_v / 10, soil_v % 10,
			mq2_v / 10, mq2_v % 10,
			mode_text,
			AutoControl_GetPumpState(),
			AutoControl_GetFanState(),
			AutoControl_GetFillState(),
			AutoControl_GetServoAngle(),
			AutoControl_GetAlarmFlags(),
			th->temp_open_c, th->temp_close_c,
			th->light_on_x10, th->light_off_x10,
			th->soil_on_x10, th->soil_off_x10,
			th->mq2_on_x10, th->mq2_off_x10);
	}

	len = (uint16_t)strlen(json);
	if (len + 2 >= sizeof(ESP_PublishJson))
	{
		return 0;
	}

	json[len] = '\r';
	json[len + 1] = '\n';

	Serial_SendBuffer(json, len + 2);
	while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET)
	{
	}

	return 1;
}

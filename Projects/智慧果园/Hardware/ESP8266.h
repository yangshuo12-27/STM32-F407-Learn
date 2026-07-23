#ifndef __ESP8266_H
#define __ESP8266_H

#include "stm32f10x.h"
#include "AutoControl.h"

typedef enum
{
	ESP_WIFI_IDLE = 0,
	ESP_WIFI_CONNECTING,
	ESP_WIFI_OK,
	ESP_WIFI_FAIL,
	ESP_MQTT_OK,
	ESP_MQTT_FAIL
} ESP_Status_t;

void ESP8266_Init(void);
uint8_t ESP8266_TCP_Connect(void);
uint8_t ESP8266_TCP_Reconnect(void);
uint8_t ESP8266_PublishSensorData(const SensorSnapshot_t *sensor);
ESP_Status_t ESP8266_GetStatus(void);
const char *ESP8266_GetStatusText(void);
void ESP8266_Process(void);
uint8_t ESP8266_IsBusy(void);
uint8_t ESP8266_IsConnected(void);
void ESP8266_ForceDisconnect(void);

#endif

#include "stm32f10x.h"
#include "Delay.h"
#include "Relay.h"
#include "Key.h"
#include "LCD.h"
#include "Buzzer.h"
#include "Servo.h"
#include "DHT11.h"
#include "SensorADC.h"
#include "Serial.h"
#include "ESP8266.h"
#include "FillLight.h"
#include "WiFiConfig.h"
#include "AutoControl.h"
#include "AT24C02.h"
#include "UI.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define START_TASK_PRIO         1
#define START_STK_SIZE          256
#define CONTROL_TASK_PRIO       3
#define CONTROL_STK_SIZE        384
#define COMM_TASK_PRIO          4
#define COMM_STK_SIZE           768
#define MQTT_RECONNECT_TICK     300

#define LOOP_MS                 10
#define LCD_PERIOD_TICK         100
#define AUTO_PERIOD_TICK        100
#define DHT_PERIOD_TICK         200

static TaskHandle_t StartTask_Handler;
static TaskHandle_t ControlTask_Handler;
static TaskHandle_t CommTask_Handler;
static SemaphoreHandle_t Sensor_Mutex;

static DHT11_Data_TypeDef DHT11_Data;
static uint8_t DHT11_OK;
static uint32_t System_Tick;
static SensorSnapshot_t g_sensor;

static void BuildSensorSnapshot(SensorSnapshot_t *sensor)
{
	sensor->dht_ok = DHT11_OK;
	sensor->temp_int = DHT11_Data.temp_int;
	sensor->temp_deci = DHT11_Data.temp_deci;
	sensor->humi_int = DHT11_Data.humi_int;
	sensor->light_raw = SensorADC_GetLight();
	sensor->soil_raw = SensorADC_GetSoil();
	sensor->mq2_raw = SensorADC_GetMQ2();
}

static void SensorSnapshot_Update(void)
{
	xSemaphoreTake(Sensor_Mutex, portMAX_DELAY);
	BuildSensorSnapshot(&g_sensor);
	xSemaphoreGive(Sensor_Mutex);
}

static void SensorSnapshot_Copy(SensorSnapshot_t *sensor)
{
	xSemaphoreTake(Sensor_Mutex, portMAX_DELAY);
	*sensor = g_sensor;
	xSemaphoreGive(Sensor_Mutex);
}

static void Hardware_Init(void)
{
	Relay_Init();
	FillLight_Init();
	Buzzer_Init();
	Lcd_Init();
	Key_Init();
	Servo_Init();
	DHT11_Init();
	SensorADC_Init();
	Serial_Init();
	ESP8266_Init();
	AT24C02_Init();
	AutoControl_Init();
	UI_Init();

	Lcd_Clear(BLACK);

	DHT11_OK = Read_DHT11(&DHT11_Data);
	SensorADC_Update();
	BuildSensorSnapshot(&g_sensor);
	AutoControl_UpdateAuto(&g_sensor);

	UI_ForceRedraw();
	UI_Update(&g_sensor);
}

static void control_task(void *pvParameters)
{
	uint8_t lcd_tick = 0;
	uint8_t auto_tick = 0;
	uint8_t dht_tick = 0;
	KeyEvent_t key_event;
	SensorSnapshot_t sensor;

	(void)pvParameters;

	while (1)
	{
		System_Tick++;
		UI_SetTick(System_Tick);

		Key_Update();
		while (1)
		{
			key_event = Key_GetEvent();
			if (key_event == KEY_EVENT_NONE)
			{
				break;
			}
			UI_OnKey(key_event);
			SensorSnapshot_Copy(&sensor);
			UI_Update(&sensor);
		}

		if (++dht_tick >= DHT_PERIOD_TICK)
		{
			dht_tick = 0;
			if (ESP8266_IsBusy() == 0)
			{
				DHT11_OK = Read_DHT11(&DHT11_Data);
				SensorSnapshot_Update();
			}
		}

		if (++auto_tick >= AUTO_PERIOD_TICK)
		{
			auto_tick = 0;
			SensorADC_Update();
			SensorSnapshot_Update();
			if (AutoControl_GetMode() == RUN_MODE_AUTO)
			{
				SensorSnapshot_Copy(&sensor);
				AutoControl_UpdateAuto(&sensor);
			}
			if (AutoControl_GetAlarmFlags() != 0)
			{
				Buzzer_Beep(80);
			}
		}

		if (++lcd_tick >= LCD_PERIOD_TICK)
		{
			lcd_tick = 0;
			SensorSnapshot_Copy(&sensor);
			UI_Update(&sensor);
		}

		vTaskDelay(pdMS_TO_TICKS(LOOP_MS));
	}
}

static void comm_task(void *pvParameters)
{
	TickType_t publish_wake = xTaskGetTickCount();
	uint8_t was_connected = 0;
	SensorSnapshot_t sensor;

	(void)pvParameters;

	vTaskDelay(pdMS_TO_TICKS(500));

	while (1)
	{
		if (ESP8266_IsConnected() == 0)
		{
			was_connected = 0;
			ESP8266_Process();
			if (ESP8266_IsBusy() == 0)
			{
				ESP8266_TCP_Connect();
			}
			vTaskDelay(pdMS_TO_TICKS(1000));
			continue;
		}

		if (was_connected == 0)
		{
			was_connected = 1;
			publish_wake = xTaskGetTickCount();
			Buzzer_Beep(200);
		}

		ESP8266_Process();

		if ((TickType_t)(xTaskGetTickCount() - publish_wake) >= pdMS_TO_TICKS(TCP_PUBLISH_PERIOD_MS))
		{
			publish_wake = xTaskGetTickCount();
			SensorSnapshot_Update();
			SensorSnapshot_Copy(&sensor);
			(void)ESP8266_PublishSensorData(&sensor);
		}

		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

static void start_task(void *pvParameters)
{
	(void)pvParameters;

	taskENTER_CRITICAL();

	Sensor_Mutex = xSemaphoreCreateMutex();

	xTaskCreate((TaskFunction_t)control_task,
	            (const char *)"control",
	            (uint16_t)CONTROL_STK_SIZE,
	            (void *)NULL,
	            (UBaseType_t)CONTROL_TASK_PRIO,
	            (TaskHandle_t *)&ControlTask_Handler);

	xTaskCreate((TaskFunction_t)comm_task,
	            (const char *)"comm",
	            (uint16_t)COMM_STK_SIZE,
	            (void *)NULL,
	            (UBaseType_t)COMM_TASK_PRIO,
	            (TaskHandle_t *)&CommTask_Handler);

	vTaskDelete(StartTask_Handler);
	taskEXIT_CRITICAL();
}

int main(void)
{
	Hardware_Init();

	xTaskCreate((TaskFunction_t)start_task,
	            (const char *)"start",
	            (uint16_t)START_STK_SIZE,
	            (void *)NULL,
	            (UBaseType_t)START_TASK_PRIO,
	            (TaskHandle_t *)&StartTask_Handler);

	vTaskStartScheduler();

	while (1)
	{
	}
}

#include "UI.h"
#include "LCD.h"
#include "Font.h"
#include "SensorADC.h"
#include "ESP8266.h"
#include <stdio.h>
#include <string.h>

#define UI_TCP_STATUS_Y         96

static UIPage_t UI_Page = UI_PAGE_ENV1;
static UIPage_t UI_DrawnPage = (UIPage_t)0xFF;
static uint8_t UI_SetSel;
static uint8_t UI_ManSel;
static uint8_t UI_RedrawAll;

static void UI_DrawCN(u16 x, u16 y, u16 fc, u16 bc, uint8_t idx)
{
	LCD_ShowChineseFont(x, y, 16, (u8 *)HZ_FONT_16[idx], fc, bc);
}

static void UI_DrawCNText(u16 x, u16 y, u16 fc, u16 bc, const uint8_t *idx, uint8_t count)
{
	uint8_t i;

	for (i = 0; i < count; i++)
	{
		UI_DrawCN((u16)(x + i * 16), y, fc, bc, idx[i]);
	}
}

static void UI_DrawAscii(u16 x, u16 y, u16 fc, u16 bc, const char *text)
{
	Gui_DrawFont_GBK16(x, y, fc, bc, (u8 *)text);
}

static void UI_DrawLabel(u16 y, const char *label)
{
	UI_DrawAscii(0, y, GRAY1, BLACK, label);
}

static void UI_DrawCNLabel(u16 y, const uint8_t *idx, uint8_t count)
{
	LCD_FillRect(0, y, (u16)(count * 16), 16, BLACK);
	UI_DrawCNText(0, y, GRAY1, BLACK, idx, count);
}

static void UI_DrawCNValue(u16 y, u16 fc, const uint8_t *idx, uint8_t count)
{
	LCD_FillRect(48, y, 80, 16, BLACK);
	UI_DrawCNText(48, y, fc, BLACK, idx, count);
}

static void UI_DrawValue(u16 y, u16 fc, const char *text)
{
	LCD_FillRect(48, y, 80, 16, BLACK);
	UI_DrawAscii(48, y, fc, BLACK, text);
}

static void UI_DrawFooter(const char *hint)
{
	LCD_FillRect(0, 144, 128, 16, BLACK);
	UI_DrawAscii(0, 144, GRAY1, BLACK, hint);
}

static void UI_DrawFooterPage(void)
{
	LCD_FillRect(0, 144, 128, 16, BLACK);
	UI_DrawAscii(0, 144, GRAY1, BLACK, "K2");
	UI_DrawCNText(16, 144, GRAY1, BLACK, (const uint8_t[]){FONT_FAN, FONT_YE}, 2);
}

static void UI_DrawFooterAdjust(void)
{
	LCD_FillRect(0, 144, 128, 16, BLACK);
	UI_DrawAscii(0, 144, GRAY1, BLACK, "K1");
	UI_DrawCNText(16, 144, GRAY1, BLACK, (const uint8_t[]){FONT_XUAN}, 1);
	UI_DrawAscii(32, 144, GRAY1, BLACK, " K3+ K4-");
}

static void UI_DrawTcpStatus(void)
{
	const char *text = ESP8266_GetStatusText();
	u16 fc = GRAY1;
	ESP_Status_t status = ESP8266_GetStatus();

	if (ESP8266_IsConnected() != 0)
	{
		fc = GREEN;
	}
	else if (status == ESP_WIFI_FAIL || status == ESP_MQTT_FAIL)
	{
		fc = RED;
	}
	else if (status == ESP_WIFI_CONNECTING)
	{
		fc = YELLOW;
	}

	LCD_FillRect(0, UI_TCP_STATUS_Y, 128, 16, BLACK);
	UI_DrawAscii(0, UI_TCP_STATUS_Y, fc, BLACK, "TCP:");
	UI_DrawAscii(32, UI_TCP_STATUS_Y, fc, BLACK, text);
}

static void UI_DrawPageNum(void)
{
	char buf[8];

	sprintf(buf, "%u/%u", (unsigned)(UI_Page + 1), (unsigned)UI_PAGE_COUNT);
	UI_DrawAscii(100, 0, GRAY1, BLACK, buf);
}

static void UI_FormatVolt(char *buf, uint8_t volt_x10)
{
	sprintf(buf, "%u.%uV", volt_x10 / 10, volt_x10 % 10);
}

static void UI_DrawEnv1Static(void)
{
	UI_DrawCNText(32, 0, YELLOW, BLACK, (const uint8_t[]){FONT_ZHI, FONT_HUI, FONT_GUO, FONT_YUAN}, 4);
	UI_DrawCNText(32, 18, WHITE, BLACK, (const uint8_t[]){FONT_HUAN, FONT_JING, FONT_JIAN, FONT_CE}, 4);
	UI_DrawPageNum();
	UI_DrawCNLabel(40, (const uint8_t[]){FONT_WEN, FONT_DU}, 2);
	UI_DrawCNLabel(58, (const uint8_t[]){FONT_SHI, FONT_DU}, 2);
	UI_DrawCNLabel(76, (const uint8_t[]){FONT_GUANG, FONT_ZHAO}, 2);
	UI_DrawFooterPage();
}

static void UI_DrawEnv2Static(void)
{
	UI_DrawCNText(32, 0, YELLOW, BLACK, (const uint8_t[]){FONT_HUAN, FONT_JING, FONT_SHU, FONT_JU}, 4);
	UI_DrawPageNum();
	UI_DrawCNLabel(40, (const uint8_t[]){FONT_TU, FONT_RANG}, 2);
	UI_DrawCNLabel(58, (const uint8_t[]){FONT_YAN, FONT_WU}, 2);
	UI_DrawCNLabel(76, (const uint8_t[]){FONT_SHUI, FONT_BENG}, 2);
	UI_DrawCNLabel(94, (const uint8_t[]){FONT_FENG, FONT_SHAN}, 2);
	UI_DrawCNLabel(112, (const uint8_t[]){FONT_BU, FONT_GUANG}, 2);
	UI_DrawFooterPage();
}

static void UI_DrawSettingStatic(void)
{
	UI_DrawCNText(32, 0, YELLOW, BLACK, (const uint8_t[]){FONT_YU_T, FONT_ZHI_V, FONT_SHE, FONT_ZHI_S}, 4);
	UI_DrawPageNum();
	UI_DrawCNLabel(36, (const uint8_t[]){FONT_WEN, FONT_GAO}, 2);
	UI_DrawCNLabel(52, (const uint8_t[]){FONT_WEN, FONT_DI}, 2);
	UI_DrawCNLabel(68, (const uint8_t[]){FONT_GUANG, FONT_KAI}, 2);
	UI_DrawCNLabel(84, (const uint8_t[]){FONT_GUANG, FONT_GUAN}, 2);
	UI_DrawCNLabel(100, (const uint8_t[]){FONT_TU, FONT_GAO}, 2);
	UI_DrawCNLabel(116, (const uint8_t[]){FONT_TU, FONT_DI}, 2);
	UI_DrawFooterAdjust();
}

static void UI_DrawModeStatic(void)
{
	UI_DrawCNText(32, 0, YELLOW, BLACK, (const uint8_t[]){FONT_YUN, FONT_XING, FONT_MO, FONT_SHI_T}, 4);
	UI_DrawPageNum();
	UI_DrawCNText(0, 40, WHITE, BLACK, (const uint8_t[]){FONT_DANG, FONT_QIAN}, 2);
	UI_DrawCNText(0, 72, GRAY1, BLACK, (const uint8_t[]){FONT_ZI, FONT_DONG, FONT_MO, FONT_SHI_T}, 4);
	UI_DrawCNText(0, 88, GRAY1, BLACK, (const uint8_t[]){FONT_SHOU, FONT_DONG, FONT_MO, FONT_SHI_T}, 4);
	UI_DrawAscii(0, 112, WHITE, BLACK, "K1");
	UI_DrawCNText(16, 112, WHITE, BLACK, (const uint8_t[]){FONT_QIE, FONT_HUAN2}, 2);
	UI_DrawFooterPage();
}

static void UI_DrawCtrlStatic(void)
{
	UI_DrawCNText(32, 0, YELLOW, BLACK, (const uint8_t[]){FONT_SHOU, FONT_DONG, FONT_KONG, FONT_ZHI_C}, 4);
	UI_DrawPageNum();
	UI_DrawCNLabel(40, (const uint8_t[]){FONT_CHUANG, FONT_HU}, 2);
	UI_DrawCNLabel(58, (const uint8_t[]){FONT_BU, FONT_GUANG}, 2);
	UI_DrawCNLabel(76, (const uint8_t[]){FONT_SHUI, FONT_BENG}, 2);
	UI_DrawCNLabel(94, (const uint8_t[]){FONT_FENG, FONT_SHAN}, 2);
	UI_DrawFooterAdjust();
}

static void UI_DrawPageStatic(UIPage_t page)
{
	Lcd_Clear(BLACK);
	switch (page)
	{
	case UI_PAGE_ENV1:
		UI_DrawEnv1Static();
		break;
	case UI_PAGE_ENV2:
		UI_DrawEnv2Static();
		break;
	case UI_PAGE_SETTING:
		UI_DrawSettingStatic();
		break;
	case UI_PAGE_MODE:
		UI_DrawModeStatic();
		break;
	case UI_PAGE_CTRL:
		UI_DrawCtrlStatic();
		break;
	default:
		break;
	}
}

static void UI_DrawEnv1Dynamic(const SensorSnapshot_t *sensor)
{
	AutoThreshold_t *th = AutoControl_GetThreshold();
	char buf[16];
	uint8_t alarm = AutoControl_GetAlarmFlags();

	if (sensor->dht_ok != 0)
	{
		uint8_t temp_alarm = (alarm & (ALARM_TEMP_HIGH | ALARM_TEMP_LOW)) ? 1 : 0;
		sprintf(buf, "%u.%uC", sensor->temp_int, sensor->temp_deci);
		UI_DrawValue(40, temp_alarm ? RED : YELLOW, buf);
		sprintf(buf, "%u%%", sensor->humi_int);
		UI_DrawValue(58, GREEN, buf);
	}
	else
	{
		UI_DrawValue(40, RED, "ERR");
		UI_DrawValue(58, RED, "---");
	}

	UI_FormatVolt(buf, (uint8_t)SensorADC_ToVolt_x10(sensor->light_raw));
	UI_DrawValue(76, (SensorADC_ToVolt_x10(sensor->light_raw) > th->light_on_x10) ? GREEN : WHITE, buf);
	UI_DrawTcpStatus();
}

static void UI_DrawOnOffValue(u16 y, u16 fc, uint8_t on)
{
	if (on != 0)
	{
		UI_DrawCNValue(y, fc, (const uint8_t[]){FONT_KAI}, 1);
	}
	else
	{
		UI_DrawCNValue(y, fc, (const uint8_t[]){FONT_GUAN}, 1);
	}
}

static void UI_DrawEnv2Dynamic(const SensorSnapshot_t *sensor)
{
	char buf[16];
	AutoThreshold_t *th = AutoControl_GetThreshold();

	UI_FormatVolt(buf, (uint8_t)SensorADC_ToVolt_x10(sensor->soil_raw));
	UI_DrawValue(40, (SensorADC_ToVolt_x10(sensor->soil_raw) > th->soil_on_x10) ? RED : GREEN, buf);

	UI_FormatVolt(buf, (uint8_t)SensorADC_ToVolt_x10(sensor->mq2_raw));
	UI_DrawValue(58, (SensorADC_ToVolt_x10(sensor->mq2_raw) > th->mq2_on_x10) ? RED : WHITE, buf);

	UI_DrawOnOffValue(76, AutoControl_GetPumpState() ? GREEN : GRAY1, AutoControl_GetPumpState());
	UI_DrawOnOffValue(94, AutoControl_GetFanState() ? GREEN : GRAY1, AutoControl_GetFanState());
	UI_DrawOnOffValue(112, AutoControl_GetFillState() ? GREEN : GRAY1, AutoControl_GetFillState());
}

static void UI_DrawSettingDynamic(void)
{
	AutoThreshold_t *th = AutoControl_GetThreshold();
	char buf[16];
	static const u16 row_y[] = {36, 52, 68, 84, 100, 116};
	uint8_t i;

	LCD_FillRect(38, 36, 8, 96, BLACK);
	UI_DrawAscii(38, row_y[UI_SetSel], YELLOW, BLACK, ">");

	for (i = 0; i < 6; i++)
	{
		u16 fc = (i == UI_SetSel) ? YELLOW : WHITE;

		if (i == 0)
		{
			sprintf(buf, "%2uC", th->temp_open_c);
		}
		else if (i == 1)
		{
			sprintf(buf, "%2uC", th->temp_close_c);
		}
		else if (i == 2)
		{
			UI_FormatVolt(buf, th->light_on_x10);
		}
		else if (i == 3)
		{
			UI_FormatVolt(buf, th->light_off_x10);
		}
		else if (i == 4)
		{
			UI_FormatVolt(buf, th->soil_on_x10);
		}
		else
		{
			UI_FormatVolt(buf, th->soil_off_x10);
		}
		UI_DrawValue(row_y[i], fc, buf);
	}
}

static void UI_DrawModeDynamic(void)
{
	if (AutoControl_GetMode() == RUN_MODE_AUTO)
	{
		UI_DrawCNValue(40, GREEN, (const uint8_t[]){FONT_ZI, FONT_DONG}, 2);
	}
	else
	{
		UI_DrawCNValue(40, YELLOW, (const uint8_t[]){FONT_SHOU, FONT_DONG}, 2);
	}
}

static void UI_DrawCtrlDynamic(void)
{
	char buf[16];
	static const u16 row_y[] = {40, 58, 76, 94};
	u16 active_fc;
	u16 i;

	LCD_FillRect(38, 40, 8, 70, BLACK);
	UI_DrawAscii(38, row_y[UI_ManSel], YELLOW, BLACK, ">");

	for (i = 0; i < 4; i++)
	{
		if (i == UI_ManSel)
		{
			active_fc = YELLOW;
		}
		else if (AutoControl_GetMode() == RUN_MODE_MANUAL)
		{
			active_fc = WHITE;
		}
		else
		{
			active_fc = GRAY1;
		}

		if (i == 0)
		{
			sprintf(buf, "%3u", AutoControl_GetServoAngle());
			UI_DrawValue(row_y[0], active_fc, buf);
		}
		else if (i == 1)
		{
			UI_DrawOnOffValue(row_y[1], active_fc, AutoControl_GetFillState());
		}
		else if (i == 2)
		{
			UI_DrawOnOffValue(row_y[2], active_fc, AutoControl_GetPumpState());
		}
		else
		{
			UI_DrawOnOffValue(row_y[3], active_fc, AutoControl_GetFanState());
		}
	}
}

static void UI_DrawPageDynamic(const SensorSnapshot_t *sensor)
{
	switch (UI_Page)
	{
	case UI_PAGE_ENV1:
		UI_DrawEnv1Dynamic(sensor);
		break;
	case UI_PAGE_ENV2:
		UI_DrawEnv2Dynamic(sensor);
		break;
	case UI_PAGE_SETTING:
		UI_DrawSettingDynamic();
		break;
	case UI_PAGE_MODE:
		UI_DrawModeDynamic();
		break;
	case UI_PAGE_CTRL:
		UI_DrawCtrlDynamic();
		break;
	default:
		break;
	}
}

void UI_Init(void)
{
	UI_Page = UI_PAGE_ENV1;
	UI_DrawnPage = (UIPage_t)0xFF;
	UI_SetSel = 0;
	UI_ManSel = 0;
	UI_RedrawAll = 1;
}

void UI_SetTick(uint32_t tick)
{
	(void)tick;
}

void UI_ForceRedraw(void)
{
	UI_RedrawAll = 1;
}

void UI_OnKey(KeyEvent_t event)
{
	uint8_t angle;

	if (event == KEY_EVENT_NONE)
	{
		return;
	}

	if (event == KEY_EVENT_K1)
	{
		if (UI_Page == UI_PAGE_SETTING)
		{
			UI_SetSel = (UI_SetSel + 1) % 6;
		}
		else if (UI_Page == UI_PAGE_CTRL)
		{
			UI_ManSel = (UI_ManSel + 1) % 4;
		}
		else
		{
			AutoControl_ToggleMode();
			UI_RedrawAll = 1;
		}
		return;
	}

	if (event == KEY_EVENT_K2)
	{
		UI_Page = (UIPage_t)((UI_Page + 1) % UI_PAGE_COUNT);
		UI_RedrawAll = 1;
		return;
	}

	if (UI_Page == UI_PAGE_SETTING)
	{
		if (event == KEY_EVENT_K3)
		{
			AutoControl_AdjustThreshold(UI_SetSel, 1);
		}
		else if (event == KEY_EVENT_K4)
		{
			AutoControl_AdjustThreshold(UI_SetSel, -1);
		}
		return;
	}

	if (UI_Page == UI_PAGE_CTRL && AutoControl_GetMode() == RUN_MODE_MANUAL)
	{
		if (event == KEY_EVENT_K3)
		{
			if (UI_ManSel == 0)
			{
				angle = AutoControl_GetServoAngle() + 15;
				if (angle > 180)
				{
					angle = 180;
				}
				AutoControl_ApplyManual(-1, -1, -1, angle);
			}
			else if (UI_ManSel == 1)
			{
				AutoControl_ApplyManual(-1, -1, 1, -1);
			}
			else if (UI_ManSel == 2)
			{
				AutoControl_ApplyManual(1, -1, -1, -1);
			}
			else
			{
				AutoControl_ApplyManual(-1, 1, -1, -1);
			}
		}
		else if (event == KEY_EVENT_K4)
		{
			if (UI_ManSel == 0)
			{
				angle = AutoControl_GetServoAngle();
				if (angle >= 15)
				{
					angle -= 15;
				}
				else
				{
					angle = 0;
				}
				AutoControl_ApplyManual(-1, -1, -1, angle);
			}
			else if (UI_ManSel == 1)
			{
				AutoControl_ApplyManual(-1, -1, 0, -1);
			}
			else if (UI_ManSel == 2)
			{
				AutoControl_ApplyManual(0, -1, -1, -1);
			}
			else
			{
				AutoControl_ApplyManual(-1, 0, -1, -1);
			}
		}
	}
}

void UI_Update(const SensorSnapshot_t *sensor)
{
	if (UI_RedrawAll != 0 || UI_Page != UI_DrawnPage)
	{
		UI_DrawPageStatic(UI_Page);
		UI_DrawnPage = UI_Page;
		UI_RedrawAll = 0;
	}

	UI_DrawPageDynamic(sensor);
}

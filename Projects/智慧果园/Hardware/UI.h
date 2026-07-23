#ifndef __UI_H
#define __UI_H

#include "AutoControl.h"
#include "Key.h"

typedef enum
{
	UI_PAGE_ENV1 = 0,
	UI_PAGE_ENV2,
	UI_PAGE_SETTING,
	UI_PAGE_MODE,
	UI_PAGE_CTRL,
	UI_PAGE_COUNT
} UIPage_t;

void UI_Init(void);
void UI_SetTick(uint32_t tick);
void UI_OnKey(KeyEvent_t event);
void UI_ForceRedraw(void);
void UI_Update(const SensorSnapshot_t *sensor);

#endif

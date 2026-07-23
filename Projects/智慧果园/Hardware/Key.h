#ifndef __KEY_H
#define __KEY_H

typedef enum
{
	KEY_EVENT_NONE = 0,
	KEY_EVENT_K1,
	KEY_EVENT_K2,
	KEY_EVENT_K3,
	KEY_EVENT_K4,
	KEY_EVENT_K5
} KeyEvent_t;

void Key_Init(void);
void Key_Update(void);
KeyEvent_t Key_GetEvent(void);

#endif

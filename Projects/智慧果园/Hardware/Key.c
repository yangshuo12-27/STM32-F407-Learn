#include "stm32f10x.h"
#include "Key.h"

#define KEY_DEBOUNCE    3
#define KEY_COUNT       5
#define KEY_Q_SIZE      4

static KeyEvent_t Key_Queue[KEY_Q_SIZE];
static uint8_t Key_QHead;
static uint8_t Key_QTail;
static uint8_t Key_Cnt[KEY_COUNT];
static uint8_t Key_Fired[KEY_COUNT];

static const uint16_t Key_Pins[KEY_COUNT] = {
	GPIO_Pin_5, GPIO_Pin_6, GPIO_Pin_7, GPIO_Pin_8, GPIO_Pin_9
};

static const KeyEvent_t Key_Events[KEY_COUNT] = {
	KEY_EVENT_K1, KEY_EVENT_K2, KEY_EVENT_K3, KEY_EVENT_K4, KEY_EVENT_K5
};

static void Key_Enqueue(KeyEvent_t event)
{
	uint8_t next;

	if (event == KEY_EVENT_NONE)
	{
		return;
	}

	next = (uint8_t)((Key_QTail + 1) % KEY_Q_SIZE);
	if (next == Key_QHead)
	{
		return;
	}

	Key_Queue[Key_QTail] = event;
	Key_QTail = next;
}

void Key_Init(void)
{
	uint8_t i;
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	Key_QHead = 0;
	Key_QTail = 0;
	for (i = 0; i < KEY_COUNT; i++)
	{
		Key_Cnt[i] = 0;
		Key_Fired[i] = 0;
	}
}

void Key_Update(void)
{
	uint8_t i;
	uint8_t now;

	for (i = 0; i < KEY_COUNT; i++)
	{
		now = (GPIO_ReadInputDataBit(GPIOB, Key_Pins[i]) == 0) ? 1 : 0;
		if (now != 0)
		{
			if (Key_Cnt[i] < 255)
			{
				Key_Cnt[i]++;
			}
			if (Key_Cnt[i] >= KEY_DEBOUNCE && Key_Fired[i] == 0)
			{
				Key_Fired[i] = 1;
				Key_Enqueue(Key_Events[i]);
			}
		}
		else
		{
			Key_Cnt[i] = 0;
			Key_Fired[i] = 0;
		}
	}
}

KeyEvent_t Key_GetEvent(void)
{
	KeyEvent_t event = KEY_EVENT_NONE;

	if (Key_QHead != Key_QTail)
	{
		event = Key_Queue[Key_QHead];
		Key_QHead = (uint8_t)((Key_QHead + 1) % KEY_Q_SIZE);
	}

	return event;
}

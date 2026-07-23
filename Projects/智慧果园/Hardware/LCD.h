#ifndef __LCD_H
#define __LCD_H

#include "stm32f10x.h"

#define RED  	0xf800
#define GREEN	0x07e0
#define BLUE 	0x001f
#define WHITE	0xffff
#define BLACK	0x0000
#define YELLOW  0xFFE0
#define GRAY0   0xEF7D
#define GRAY1   0x8410
#define NAVY    0x0010
#define LINE_W  WHITE
#define LINE_G  GRAY1

#define X_MAX_PIXEL	128
#define Y_MAX_PIXEL	160

/* 原理图 LCD1 引脚: SCL-PB13 SDA-PB15 RES-PB14 DC-PB1 CS-PB12 BL-PA8 */
#define LCD_CTRLA    GPIOA
#define LCD_CTRLB    GPIOB

#define LCD_SCL     GPIO_Pin_13
#define LCD_SDA     GPIO_Pin_15
#define LCD_CS      GPIO_Pin_12
#define LCD_RS      GPIO_Pin_1
#define LCD_RST     GPIO_Pin_14
#define LCD_LED     GPIO_Pin_8

#define LCD_SCL_SET   LCD_CTRLB->BSRR = LCD_SCL
#define LCD_SDA_SET   LCD_CTRLB->BSRR = LCD_SDA
#define LCD_CS_SET    LCD_CTRLB->BSRR = LCD_CS
#define LCD_RS_SET    LCD_CTRLB->BSRR = LCD_RS
#define LCD_RST_SET   LCD_CTRLB->BSRR = LCD_RST
#define LCD_LED_SET   LCD_CTRLA->BSRR = LCD_LED

#define LCD_SCL_CLR   LCD_CTRLB->BRR = LCD_SCL
#define LCD_SDA_CLR   LCD_CTRLB->BRR = LCD_SDA
#define LCD_CS_CLR    LCD_CTRLB->BRR = LCD_CS
#define LCD_RS_CLR    LCD_CTRLB->BRR = LCD_RS
#define LCD_RST_CLR   LCD_CTRLB->BRR = LCD_RST
#define LCD_LED_CLR   LCD_CTRLA->BRR = LCD_LED

void LCD_GPIO_Init(void);
void Lcd_WriteIndex(u8 Index);
void Lcd_WriteData(u8 Data);
void Lcd_WriteReg(u8 Index, u8 Data);
void Lcd_Reset(void);
void Lcd_Init(void);
void Lcd_Clear(u16 Color);
void LCD_FillRect(u16 x, u16 y, u16 w, u16 h, u16 color);
void Lcd_SetXY(u16 x, u16 y);
void Gui_DrawPoint(u16 x, u16 y, u16 Data);
void Lcd_SetRegion(u16 x_start, u16 y_start, u16 x_end, u16 y_end);
void LCD_WriteData_16Bit(u16 Data);
void Gui_DrawFont_GBK16(u16 x, u16 y, u16 fc, u16 bc, u8 *s);
void LCD_ShowChineseFont(u32 x, u32 y, u32 size, u8 *font, u16 c1, u16 c2);

#endif

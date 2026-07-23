/*
 * R60ABD1 60G mmWave sleep radar project
 * Extended: DP3/DP5 position, DP7/DP9/DP10 wave & breath status, DP14~DP20 sleep analysis
 */

#include "stm32f10x.h"
#include "led.h"
#include "delay.h"
#include "key.h"
#include "usart.h"
#include <string.h>
#include "timer.h"
#include "lcd.h"
#include <stdlib.h>
#include "font.h"
#include <math.h>
#include "esp8266.h"
#include "MLX90614.h"

/*
Hardware:
 TFT: SCL-PC8 SDA-PC9 RST-PC10 DC-PB7 CS-PB8 BL-PB11(conflict with USART3 RX, check wiring)
 ESP8266: USART2 PA2/PA3
 Radar R60ABD1: USART3 PB10(TX)->RXD  PB11(RX)->TXD
 MLX90614: SCL-PA6 SDA-PA7
 BEEP: PC7
*/

#define CONNECT_WIFI        "QWER"
#define CONNECT_PASS        "12345678"
#define CONNECT_SERVER_IP   "47.113.206.147"
#define CONNECT_SERVER_PORT 7777

#define PAGE_COUNT  4

/* ---------- globals ---------- */
int breath_rate = 0;
int heart_rate = 0;
int presence = 0;
int body_status = 0;
int movement_amplitude = 0;
float body_temperature = 36.5f;

uint16_t human_distance = 0;
int16_t human_x = 0;
int16_t human_y = 0;
int16_t human_z = 0;

int breath_status = 4; /* 1 normal 2 high 3 low 4 none */

uint8_t heart_wave[5] = {128, 128, 128, 128, 128};
uint8_t breath_wave[5] = {128, 128, 128, 128, 128};
uint8_t heart_wave_avg = 128;
uint8_t breath_wave_avg = 128;

int bed_status = 0;        /* 0 leave 1 in bed */
int sleep_quality = 3;     /* 0 deep 1 light 2 awake 3 leave */
uint16_t awake_duration = 0;
uint16_t light_duration = 0;
uint16_t deep_duration = 0;
uint16_t sleep_duration = 0;

uint8_t sleep_score = 0;
uint8_t sleep_grade = 0;     /* 0 none 1 good 2 mid 3 bad */
uint8_t sleep_abnormal = 3;  /* 0 short 1 long 2 nobody 3 none */
uint8_t struggle_status = 2; /* 0 ok 1 struggle 2 none */
uint8_t noperson_status = 0; /* 0 none 1 ok 2 abnormal */

uint8_t avg_breath = 0;
uint8_t avg_heart = 0;
uint8_t turn_count = 0;
uint8_t big_move_ratio = 0;
uint8_t small_move_ratio = 0;
uint8_t apnea_count = 0;

uint16_t sleep_total_min = 0;
uint8_t awake_ratio = 0;
uint8_t light_ratio = 0;
uint8_t deep_ratio = 0;
uint8_t leave_bed_min = 0;
uint8_t leave_bed_count = 0;
uint8_t night_turn_count = 0;
uint8_t night_avg_breath = 0;
uint8_t night_avg_heart = 0;

char message[1024];

#define JTAG_SWD_DISABLE   0X02
#define SWD_ENABLE         0X01
#define JTAG_SWD_ENABLE    0X00

void JTAG_Set(u8 mode)
{
    u32 temp;
    temp = mode;
    temp <<= 25;
    RCC->APB2ENR |= 1 << 0;
    AFIO->MAPR &= 0xF8FFFFFF;
    AFIO->MAPR |= temp;
}

unsigned char *create_frame(unsigned char control, unsigned char command, unsigned char data)
{
    static unsigned char frame[10];
    frame[0] = 0x53;
    frame[1] = 0x59;
    frame[2] = control;
    frame[3] = command;
    frame[4] = 0x00;
    frame[5] = 0x01;
    frame[6] = data;
    frame[7] = (unsigned char)((frame[0] + frame[1] + frame[2] + frame[3] + frame[4] + frame[5] + frame[6]) & 0xFF);
    frame[8] = 0x54;
    frame[9] = 0x43;
    return frame;
}

static void Radar_SendCmd(unsigned char control, unsigned char command, unsigned char data)
{
    int i;
    unsigned char *p = create_frame(control, command, data);
    for (i = 0; i < 10; i++)
    {
        USARTx_SendOneByte(USART3, p[i]);
    }
    delay_ms(300);
    if (USART3_RX_FLAG)
    {
        LED2 = !LED2;
        memset(USART3_RX_BUFFER, 0, sizeof(USART3_RX_BUFFER));
        USART3_RX_CNT = 0;
        USART3_RX_FLAG = 0;
    }
}

static uint16_t Radar_GetU16BE(const unsigned char *p)
{
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

/* bit15=0 positive, bit15=1 negative */
static int16_t Radar_ParseCoord(uint8_t hi, uint8_t lo)
{
    uint16_t raw = ((uint16_t)hi << 8) | lo;
    if (raw & 0x8000)
    {
        return (int16_t)(-(raw & 0x7FFF));
    }
    return (int16_t)(raw & 0x7FFF);
}

static uint8_t Radar_WaveAvg(const uint8_t *wave)
{
    uint16_t sum = 0;
    uint8_t i;
    for (i = 0; i < 5; i++)
    {
        sum += wave[i];
    }
    return (uint8_t)(sum / 5);
}

void R60ABD1_Init(void)
{
    Radar_SendCmd(0x84, 0x00, 0x01); /* sleep monitor */
    delay_ms(400);
    Radar_SendCmd(0x80, 0x00, 0x01); /* presence */
    delay_ms(400);
    Radar_SendCmd(0x81, 0x00, 0x01); /* breath */
    delay_ms(300);
    Radar_SendCmd(0x85, 0x00, 0x01); /* heart */
    delay_ms(300);
    Radar_SendCmd(0x81, 0x0C, 0x01); /* breath wave ON */
    delay_ms(300);
    Radar_SendCmd(0x85, 0x0A, 0x01); /* heart wave ON */
    delay_ms(300);
    Radar_SendCmd(0x84, 0x13, 0x01); /* struggle ON */
    delay_ms(300);
    Radar_SendCmd(0x84, 0x14, 0x01); /* no-person timer ON */
    delay_ms(300);
    printf("R60ABD1 init done.\r\n");
}

static void LCD_Title(void)
{
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        LCD_ShowChineseFont(16 * i, 2, 16, HZ_FONT_16[i], WHITE, 0);
    }
}

void LCD_Page1(void)
{
    Lcd_Clear(0);
    LCD_Title();
    LCD_ShowChineseFont(0, 16 * 2 + 2, 16, HZ_FONT_16[8], RED, 0);
    LCD_ShowChineseFont(16, 16 * 2 + 2, 16, HZ_FONT_16[9], RED, 0);
    LCD_ShowChineseFont(32, 16 * 2 + 2, 16, HZ_FONT_16[10], RED, 0);
    LCD_ShowChineseFont(48, 16 * 2 + 2, 16, HZ_FONT_16[11], RED, 0);

    LCD_ShowChineseFont(0, 16 * 3 + 2, 16, HZ_FONT_16[12], RED, 0);
    LCD_ShowChineseFont(16, 16 * 3 + 2, 16, HZ_FONT_16[13], RED, 0);
    LCD_ShowChineseFont(32, 16 * 3 + 2, 16, HZ_FONT_16[14], RED, 0);
    LCD_ShowChineseFont(48, 16 * 3 + 2, 16, HZ_FONT_16[15], RED, 0);

    LCD_ShowChineseFont(0, 16 * 4 + 2, 16, HZ_FONT_16[16], RED, 0);
    LCD_ShowChineseFont(16, 16 * 4 + 2, 16, HZ_FONT_16[17], RED, 0);
    LCD_ShowChineseFont(32, 16 * 4 + 2, 16, HZ_FONT_16[18], RED, 0);
    LCD_ShowChineseFont(48, 16 * 4 + 2, 16, HZ_FONT_16[19], RED, 0);

    LCD_ShowChineseFont(0, 16 * 5 + 2, 16, HZ_FONT_16[20], RED, 0);
    LCD_ShowChineseFont(16, 16 * 5 + 2, 16, HZ_FONT_16[21], RED, 0);
    LCD_ShowChineseFont(32, 16 * 5 + 2, 16, HZ_FONT_16[22], RED, 0);
    LCD_ShowChineseFont(48, 16 * 5 + 2, 16, HZ_FONT_16[23], RED, 0);

    LCD_ShowChineseFont(0, 16 * 6 + 2, 16, HZ_FONT_16[24], RED, 0);
    LCD_ShowChineseFont(16, 16 * 6 + 2, 16, HZ_FONT_16[25], RED, 0);
    LCD_ShowChineseFont(32, 16 * 6 + 2, 16, HZ_FONT_16[26], RED, 0);
    LCD_ShowChineseFont(48, 16 * 6 + 2, 16, HZ_FONT_16[27], RED, 0);

    LCD_ShowChineseFont(0, 16 * 7 + 2, 16, HZ_FONT_16[28], RED, 0);
    LCD_ShowChineseFont(16, 16 * 7 + 2, 16, HZ_FONT_16[29], RED, 0);
}

void LCD_Page2(void)
{
    Lcd_Clear(0);
    LCD_Title();
    LCD_ShowChineseFont(0, 16 * 2 + 2, 16, HZ_FONT_16[30], RED, 0);
    LCD_ShowChineseFont(16, 16 * 2 + 2, 16, HZ_FONT_16[31], RED, 0);
    LCD_ShowChineseFont(32, 16 * 2 + 2, 16, HZ_FONT_16[32], RED, 0);
    LCD_ShowChineseFont(48, 16 * 2 + 2, 16, HZ_FONT_16[33], RED, 0);

    LCD_ShowChineseFont(0, 16 * 5 + 2, 16, HZ_FONT_16[34], RED, 0);
    LCD_ShowChineseFont(16, 16 * 5 + 2, 16, HZ_FONT_16[35], RED, 0);
    LCD_ShowChineseFont(32, 16 * 5 + 2, 16, HZ_FONT_16[36], RED, 0);
    LCD_ShowChineseFont(48, 16 * 5 + 2, 16, HZ_FONT_16[37], RED, 0);
}

void LCD_Page3(void)
{
    Lcd_Clear(0);
    Gui_DrawFont_GBK16(0, 2, WHITE, 0, (u8 *)"Pos/Breath");
    Gui_DrawFont_GBK16(0, 16 * 2 + 2, RED, 0, (u8 *)"Dist cm");
    Gui_DrawFont_GBK16(0, 16 * 3 + 2, RED, 0, (u8 *)"X Y Z");
    Gui_DrawFont_GBK16(0, 16 * 5 + 2, RED, 0, (u8 *)"BreathSt");
    Gui_DrawFont_GBK16(0, 16 * 6 + 2, RED, 0, (u8 *)"HWave RWave");
    Gui_DrawFont_GBK16(0, 16 * 8 + 2, GRAY1, 0, (u8 *)"Page3/4");
}

void LCD_Page4(void)
{
    Lcd_Clear(0);
    Gui_DrawFont_GBK16(0, 2, WHITE, 0, (u8 *)"Sleep Report");
    Gui_DrawFont_GBK16(0, 16 * 2 + 2, RED, 0, (u8 *)"Score Grade");
    Gui_DrawFont_GBK16(0, 16 * 3 + 2, RED, 0, (u8 *)"A/L/D min");
    Gui_DrawFont_GBK16(0, 16 * 4 + 2, RED, 0, (u8 *)"AvgBR AvgHR");
    Gui_DrawFont_GBK16(0, 16 * 5 + 2, RED, 0, (u8 *)"Turn Big/Sm");
    Gui_DrawFont_GBK16(0, 16 * 6 + 2, RED, 0, (u8 *)"Abn Strug NP");
    Gui_DrawFont_GBK16(0, 16 * 8 + 2, GRAY1, 0, (u8 *)"Page4/4");
}

static void LCD_ShowPage(int page)
{
    if (page == 0)
        LCD_Page1();
    else if (page == 1)
        LCD_Page2();
    else if (page == 2)
        LCD_Page3();
    else
        LCD_Page4();
}

static const char *BreathStatusText(int st)
{
    switch (st)
    {
    case 1: return "OK  ";
    case 2: return "HIGH";
    case 3: return "LOW ";
    default: return "NONE";
    }
}

static const char *SleepGradeText(uint8_t g)
{
    switch (g)
    {
    case 1: return "Good";
    case 2: return "Mid ";
    case 3: return "Bad ";
    default: return "N/A ";
    }
}

static const char *SleepAbnText(uint8_t a)
{
    switch (a)
    {
    case 0: return "Short";
    case 1: return "Long ";
    case 2: return "Empty";
    default: return "None ";
    }
}

static void LCD_UpdateValues(int page)
{
    if (page == 0)
    {
        if (breath_rate > 100)
            breath_rate = 0;
        sprintf(message, "%4d", breath_rate);
        Gui_DrawFont_GBK16(80, 16 * 2 + 2, WHITE, 0, (u8 *)message);

        if (heart_rate > 200)
            heart_rate = 0;
        sprintf(message, "%4d", heart_rate);
        Gui_DrawFont_GBK16(80, 16 * 3 + 2, WHITE, 0, (u8 *)message);

        if (presence)
        {
            LCD_ShowChineseFont(80, 16 * 4 + 2, 16, HZ_FONT_16[38], WHITE, 0);
            LCD_ShowChineseFont(96, 16 * 4 + 2, 16, HZ_FONT_16[39], WHITE, 0);
        }
        else
        {
            LCD_ShowChineseFont(80, 16 * 4 + 2, 16, HZ_FONT_16[40], WHITE, 0);
            LCD_ShowChineseFont(96, 16 * 4 + 2, 16, HZ_FONT_16[41], WHITE, 0);
        }

        if (body_status)
        {
            LCD_ShowChineseFont(80, 16 * 5 + 2, 16, HZ_FONT_16[42], WHITE, 0);
            LCD_ShowChineseFont(96, 16 * 5 + 2, 16, HZ_FONT_16[43], WHITE, 0);
        }
        else
        {
            LCD_ShowChineseFont(80, 16 * 5 + 2, 16, HZ_FONT_16[44], WHITE, 0);
            LCD_ShowChineseFont(96, 16 * 5 + 2, 16, HZ_FONT_16[45], WHITE, 0);
        }

        if (movement_amplitude > 100)
            movement_amplitude = 0;
        sprintf(message, "%4d", movement_amplitude);
        Gui_DrawFont_GBK16(80, 16 * 6 + 2, WHITE, 0, (u8 *)message);

        sprintf(message, "%4.1f", body_temperature);
        Gui_DrawFont_GBK16(80, 16 * 7 + 2, WHITE, 0, (u8 *)message);
        Gui_DrawFont_GBK16(80 + 32, 16 * 7 + 2, WHITE, 0, (u8 *)"C");
    }
    else if (page == 1)
    {
        sleep_duration = (uint16_t)(awake_duration + light_duration + deep_duration);
        sprintf(message, "%4d", sleep_duration);
        Gui_DrawFont_GBK16(80, 16 * 2 + 2, WHITE, 0, (u8 *)message);
        sprintf(message, "A%u L%u D%u", awake_duration, light_duration, deep_duration);
        Gui_DrawFont_GBK16(0, 16 * 3 + 2, WHITE, 0, (u8 *)message);

        if (sleep_quality == 0)
        {
            LCD_ShowChineseFont(80, 16 * 5 + 2, 16, HZ_FONT_16[50], WHITE, 0);
            LCD_ShowChineseFont(96, 16 * 5 + 2, 16, HZ_FONT_16[51], WHITE, 0);
        }
        else if (sleep_quality == 1)
        {
            LCD_ShowChineseFont(80, 16 * 5 + 2, 16, HZ_FONT_16[52], WHITE, 0);
            LCD_ShowChineseFont(96, 16 * 5 + 2, 16, HZ_FONT_16[53], WHITE, 0);
        }
        else if (sleep_quality == 2)
        {
            LCD_ShowChineseFont(80, 16 * 5 + 2, 16, HZ_FONT_16[48], WHITE, 0);
            LCD_ShowChineseFont(96, 16 * 5 + 2, 16, HZ_FONT_16[49], WHITE, 0);
        }
        else
        {
            LCD_ShowChineseFont(80, 16 * 5 + 2, 16, HZ_FONT_16[54], WHITE, 0);
            LCD_ShowChineseFont(96, 16 * 5 + 2, 16, HZ_FONT_16[55], WHITE, 0);
        }

        sprintf(message, "Bed:%s Score:%u", bed_status ? "IN " : "OUT", sleep_score);
        Gui_DrawFont_GBK16(0, 16 * 7 + 2, WHITE, 0, (u8 *)message);
    }
    else if (page == 2)
    {
        sprintf(message, "%5u", human_distance);
        Gui_DrawFont_GBK16(72, 16 * 2 + 2, WHITE, 0, (u8 *)message);
        sprintf(message, "%d %d %d   ", human_x, human_y, human_z);
        Gui_DrawFont_GBK16(0, 16 * 4 + 2, WHITE, 0, (u8 *)message);
        Gui_DrawFont_GBK16(72, 16 * 5 + 2, WHITE, 0, (u8 *)BreathStatusText(breath_status));
        sprintf(message, "%3u  %3u", heart_wave_avg, breath_wave_avg);
        Gui_DrawFont_GBK16(0, 16 * 7 + 2, WHITE, 0, (u8 *)message);
    }
    else
    {
        sprintf(message, "%3u %s", sleep_score, SleepGradeText(sleep_grade));
        Gui_DrawFont_GBK16(0, 16 * 2 + 10, WHITE, 0, (u8 *)message);
        sprintf(message, "%u/%u/%u", awake_duration, light_duration, deep_duration);
        Gui_DrawFont_GBK16(0, 16 * 3 + 10, WHITE, 0, (u8 *)message);
        sprintf(message, "%u %u", avg_breath ? avg_breath : night_avg_breath,
                avg_heart ? avg_heart : night_avg_heart);
        Gui_DrawFont_GBK16(0, 16 * 4 + 10, WHITE, 0, (u8 *)message);
        sprintf(message, "%u %u/%u", turn_count ? turn_count : night_turn_count,
                big_move_ratio, small_move_ratio);
        Gui_DrawFont_GBK16(0, 16 * 5 + 10, WHITE, 0, (u8 *)message);
        sprintf(message, "%s %u %u", SleepAbnText(sleep_abnormal), struggle_status, noperson_status);
        Gui_DrawFont_GBK16(0, 16 * 6 + 10, WHITE, 0, (u8 *)message);
        sprintf(message, "Tot%u Leave%u", sleep_total_min, leave_bed_count);
        Gui_DrawFont_GBK16(0, 16 * 7 + 10, WHITE, 0, (u8 *)message);
    }
}

static void Radar_ParseFrame(void)
{
    uint16_t data_len;
    unsigned char *data;
    uint8_t ctrl;
    uint8_t cmd;
    uint8_t i;

    if (USART3_RX_CNT < 9)
    {
        return;
    }
    if (!(USART3_RX_BUFFER[0] == 0x53 && USART3_RX_BUFFER[1] == 0x59))
    {
        return;
    }

    ctrl = USART3_RX_BUFFER[2];
    cmd = USART3_RX_BUFFER[3];
    data_len = Radar_GetU16BE(&USART3_RX_BUFFER[4]);
    data = &USART3_RX_BUFFER[6];

    printf("type:%#X cmd:%#X len:%u d0:%#X\r\n", ctrl, cmd, data_len, data[0]);

    switch (ctrl)
    {
    case 0x80: /* presence */
        switch (cmd)
        {
        case 0x01:
            presence = (data[0] == 0x01) ? 1 : 0;
            break;
        case 0x02:
            if (data[0] == 0x02)
                body_status = 1;
            else
                body_status = 0;
            break;
        case 0x03:
            movement_amplitude = data[0];
            break;
        case 0x04: /* DP3 distance */
            if (data_len >= 2)
                human_distance = Radar_GetU16BE(data);
            else
                human_distance = data[0];
            break;
        case 0x05: /* DP5 xyz */
            if (data_len >= 6)
            {
                human_x = Radar_ParseCoord(data[0], data[1]);
                human_y = Radar_ParseCoord(data[2], data[3]);
                human_z = Radar_ParseCoord(data[4], data[5]);
            }
            break;
        default:
            break;
        }
        break;

    case 0x81: /* breath */
        switch (cmd)
        {
        case 0x01: /* DP9 */
            breath_status = data[0];
            break;
        case 0x02: /* DP8 */
            breath_rate = data[0];
            break;
        case 0x05: /* DP10 wave */
            if (data_len >= 5)
            {
                for (i = 0; i < 5; i++)
                    breath_wave[i] = data[i];
                breath_wave_avg = Radar_WaveAvg(breath_wave);
            }
            break;
        default:
            break;
        }
        break;

    case 0x84: /* sleep */
        switch (cmd)
        {
        case 0x01: /* DP11 bed */
            if (data[0] == 0x00)
            {
                bed_status = 0;
                sleep_quality = 3;
            }
            else if (data[0] == 0x01)
            {
                bed_status = 1;
            }
            break;
        case 0x02: /* DP12 */
            if (data[0] <= 0x02)
            {
                sleep_quality = data[0];
                bed_status = 1;
            }
            else
            {
                sleep_quality = 3;
                bed_status = 0;
            }
            break;
        case 0x03: /* awake duration */
            awake_duration = (data_len >= 2) ? Radar_GetU16BE(data) : data[0];
            sleep_duration = awake_duration;
            break;
        case 0x04:
            light_duration = (data_len >= 2) ? Radar_GetU16BE(data) : data[0];
            sleep_duration = light_duration;
            break;
        case 0x05:
            deep_duration = (data_len >= 2) ? Radar_GetU16BE(data) : data[0];
            sleep_duration = deep_duration;
            break;
        case 0x06: /* DP14 score */
            sleep_score = data[0];
            break;
        case 0x0C: /* DP15 composite 8B */
            if (data_len >= 8)
            {
                presence = data[0] ? 1 : 0;
                if (data[1] == 3)
                {
                    sleep_quality = 3;
                    bed_status = 0;
                }
                else
                {
                    sleep_quality = data[1];
                    bed_status = 1;
                }
                avg_breath = data[2];
                avg_heart = data[3];
                turn_count = data[4];
                big_move_ratio = data[5];
                small_move_ratio = data[6];
                apnea_count = data[7];
            }
            break;
        case 0x0D: /* DP16 night report 12B */
            if (data_len >= 12)
            {
                sleep_score = data[0];
                sleep_total_min = Radar_GetU16BE(&data[1]);
                awake_ratio = data[3];
                light_ratio = data[4];
                deep_ratio = data[5];
                leave_bed_min = data[6];
                leave_bed_count = data[7];
                night_turn_count = data[8];
                night_avg_breath = data[9];
                night_avg_heart = data[10];
                apnea_count = data[11];
            }
            break;
        case 0x0E: /* DP17 */
            sleep_abnormal = data[0];
            break;
        case 0x10: /* DP20 grade */
            sleep_grade = data[0];
            break;
        case 0x11: /* DP18 struggle */
            struggle_status = data[0];
            break;
        case 0x12: /* DP19 noperson */
            noperson_status = data[0];
            break;
        default:
            break;
        }
        break;

    case 0x85: /* heart */
        switch (cmd)
        {
        case 0x02:
            heart_rate = data[0];
            if (heart_rate > 0)
                presence = 1;
            break;
        case 0x05: /* DP7 wave */
            if (data_len >= 5)
            {
                for (i = 0; i < 5; i++)
                    heart_wave[i] = data[i];
                heart_wave_avg = Radar_WaveAvg(heart_wave);
            }
            break;
        default:
            break;
        }
        break;

    default:
        break;
    }
}

int main(void)
{
    u8 key = 0;
    u32 time_cnt = 0;
    u8 i;
    u8 run_state = 0;
    u8 esp8266_connect = 0;
    int page = 0;

    JTAG_Set(JTAG_SWD_DISABLE);

    USART1_Init(115200);
    printf("USART1 OK\r\n");
    LED_Init();
    BEEP_Init();
    KEY_Init();
    Lcd_Init();
    Lcd_Clear(0);
    LCD_LED_SET;

    USART2_Init(115200);
    TIMER2_Init(72, 20000);
    TIMER3_Init(72, 20000);
    USART3_Init(115200);

    R60ABD1_Init();
    MLX90614_Init();

    BEEP = 1;
    delay_ms(100);
    BEEP = 0;

#if 1
    Lcd_Clear(0);
    Gui_DrawFont_GBK16(0, 16 * 0 + 2, WHITE, 0, (u8 *)"WIFI Init...");
    for (i = 0; i < 10; i++)
    {
        if (ESP8266_Init() == 0)
        {
            Gui_DrawFont_GBK16(0, 16 * 0 + 2, WHITE, 0, (u8 *)"WIFI OK...");
            run_state = 1;
            break;
        }
        Gui_DrawFont_GBK16(0, 16 * 0 + 2, WHITE, 0, (u8 *)"WIFI ERROR...");
        run_state = 0;
        printf("ESP8266 error\r\n");
    }

    if (run_state)
    {
        Gui_DrawFont_GBK16(0, 16 * 1 + 2, WHITE, 0, (u8 *)"Connect WIFI...");
        Gui_DrawFont_GBK16(0, 16 * 2 + 2, WHITE, 0, (u8 *)CONNECT_WIFI);
        Gui_DrawFont_GBK16(0, 16 * 3 + 2, WHITE, 0, (u8 *)CONNECT_PASS);
AA:
        run_state = ESP8266_STA_TCP_Client_Mode(CONNECT_WIFI, CONNECT_PASS, CONNECT_SERVER_IP, CONNECT_SERVER_PORT, 1);
        if (run_state)
        {
            Gui_DrawFont_GBK16(0, 16 * 4 + 2, WHITE, 0, (u8 *)"Connect Error..");
            goto AA;
        }
        Gui_DrawFont_GBK16(0, 16 * 4 + 2, WHITE, 0, (u8 *)"Connect Success");
        delay_ms(1000);
        delay_ms(1000);
        delay_ms(1000);
        esp8266_connect = 1;
    }
#endif

    LCD_ShowPage(page);

    BEEP = 1;
    delay_ms(300);
    BEEP = 0;
    delay_ms(200);
    BEEP = 1;
    delay_ms(300);
    BEEP = 0;

    while (1)
    {
        key = KEY_Scan();
        if (key)
        {
            page = (page + 1) % PAGE_COUNT;
            LCD_ShowPage(page);
        }

        if (time_cnt > 200)
        {
            time_cnt = 0;

            body_temperature = calculate_min_average();
            if (body_temperature > 100)
            {
                body_temperature = 36.3f;
            }

            LCD_UpdateValues(page);

            /* alarm */
            if (heart_rate > 130 || body_temperature > 38.0f ||
                breath_status == 2 || breath_status == 3 ||
                sleep_abnormal == 0 || sleep_abnormal == 1 || sleep_abnormal == 2 ||
                struggle_status == 1 || noperson_status == 2)
            {
                BEEP = 0;
            }
            else
            {
                BEEP = 1;
            }

            /* JSON upload to TCP 7777??????????????????��? */
            sprintf(message,
                    "{\"device\":\"radar\","
                    "\"breath\":%d,\"heart\":%d,\"presence\":%d,\"body_status\":%d,\"move\":%d,"
                    "\"awake_min\":%u,\"light_min\":%u,\"deep_min\":%u,"
                    "\"sleep_score\":%u,\"sleep_grade\":%u,\"sleep_quality\":%d,\"bed\":%d,"
                    "\"dist\":%u,\"x\":%d,\"y\":%d,\"z\":%d,"
                    "\"breath_status\":%d,\"sleep_abn\":%u,\"struggle\":%u,\"noperson\":%u,"
                    "\"avg_breath\":%u,\"avg_heart\":%u,\"turns\":%u,\"temp\":%.1f,"
                    "\"heart_wave\":[%u,%u,%u,%u,%u],\"breath_wave\":[%u,%u,%u,%u,%u],"
                    "\"heart_wave_avg\":%u,\"breath_wave_avg\":%u,"
                    "\"sleep_duration\":%u,\"sleep_total_min\":%u,"
                    "\"leave_bed_min\":%u,\"leave_bed_count\":%u,"
                    "\"big_move_ratio\":%u,\"small_move_ratio\":%u,\"apnea_count\":%u,"
                    "\"awake_ratio\":%u,\"light_ratio\":%u,\"deep_ratio\":%u}\r\n",
                    breath_rate, heart_rate, presence, body_status, movement_amplitude,
                    awake_duration, light_duration, deep_duration,
                    sleep_score, sleep_grade, sleep_quality, bed_status,
                    human_distance, human_x, human_y, human_z,
                    breath_status, sleep_abnormal, struggle_status, noperson_status,
                    avg_breath ? avg_breath : night_avg_breath,
                    avg_heart ? avg_heart : night_avg_heart,
                    turn_count ? turn_count : night_turn_count,
                    body_temperature,
                    heart_wave[0], heart_wave[1], heart_wave[2], heart_wave[3], heart_wave[4],
                    breath_wave[0], breath_wave[1], breath_wave[2], breath_wave[3], breath_wave[4],
                    heart_wave_avg, breath_wave_avg,
                    sleep_duration, sleep_total_min,
                    leave_bed_min, leave_bed_count,
                    big_move_ratio, small_move_ratio, apnea_count,
                    awake_ratio, light_ratio, deep_ratio);
            printf("TCP: %s", message);

            if (esp8266_connect)
            {
                USARTx_StringSend(USART2, message);
            }

            LED1 = !LED1;
        }

        if (USART3_RX_FLAG)
        {
            LED2 = !LED2;
            Radar_ParseFrame();
            memset(USART3_RX_BUFFER, 0, sizeof(USART3_RX_BUFFER));
            USART3_RX_CNT = 0;
            USART3_RX_FLAG = 0;
        }

        if (USART2_RX_FLAG)
        {
            USART2_RX_BUFFER[USART2_RX_CNT] = '\0';
            printf("ESP8266 RX:\r\n%s\r\n", USART2_RX_BUFFER);
            memset(USART2_RX_BUFFER, 0, sizeof(USART2_RX_BUFFER));
            USART2_RX_CNT = 0;
            USART2_RX_FLAG = 0;
        }

        DelayMs(10);
        time_cnt++;
    }
}

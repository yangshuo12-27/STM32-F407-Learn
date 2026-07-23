#ifndef __RELAY_H
#define __RELAY_H

void Relay_Init(void);

void Relay1_ON(void);
void Relay1_OFF(void);
void Relay1_Turn(void);
uint8_t Relay1_GetState(void);

void Relay2_ON(void);
void Relay2_OFF(void);
void Relay2_Turn(void);
uint8_t Relay2_GetState(void);

#endif

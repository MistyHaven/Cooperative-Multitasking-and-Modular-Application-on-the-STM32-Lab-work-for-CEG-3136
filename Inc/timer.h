/*
* timer.h
*
*  Created on: Nov 26, 2025
*      Author: bexne
*/
#ifndef TIMER_H_
#define TIMER_H_
#include <stdint.h>
#include "stm32l5xx.h"
#include "gpio.h"
// This preprocessor macro converts a Timer register base address
// (TIM1, TIM2, TIM3, etc.) to a zero-based index (0, 1, 2, etc.)
// The address pattern can be identified from the device header file
#define TIMER_NUM(addr) ((int)addr == 0x40012C00 ? 1 : \
(int)addr == 0x40013400 ? 8 : \
(((int)addr & 0x0FC00) / 0x400 + 2))
// Timer input/output structure
typedef struct {
TIM_TypeDef *iface; // Timer peripheral
int chan; // Capture/compare channel number
Pin_t pin; // Input/output pin
int af; // Alternate function number
} TimerIO_t;
typedef enum {INCAP=1, OUTCMP=0} TimerMode_t;
typedef enum {TIPRI=0, TISEC=1, TOG=0, PWM1=1, PWM2=2} TimerSelect_t;
typedef enum {UP=0, CC1=1, CC2=2, CC3=3, CC4=4} TimerFlag_t;
void TimerEnable(TimerIO_t timer);
void TimerMode(TimerIO_t timer, TimerMode_t mode, TimerSelect_t sel);
void TimerPeriod(TimerIO_t tio, uint16_t psc, uint16_t arr, uint16_t rcr);
void TimerOutput(TimerIO_t timer, uint16_t thresh);
uint16_t TimerInput(TimerIO_t timer);
void TimerCallback(TimerIO_t timer, void (*func)(void), TimerFlag_t flag);
void TimerStart(TimerIO_t timer, TimerMode_t mode);
#endif /* TIMER_H_ */




#ifndef SYSTICK_H_
#define SYSTICK_H_
#include "stm32l5xx.h"

typedef unsigned int Time_t;
#define TIME_MAX (Time_t)(-1)

void StartSysTick();
void WaitForSysTick();
void msDelay(int t);
Time_t TimeNow();
Time_t TimePassed(Time_t since);


#endif /* SYSTICK_H_ */

#ifndef SYSCLK_H_
#define SYSCLK_H_
#include "stm32l5xx.h"
// System Clock frequency in Hz
#define SYSCLK_FREQ 48e6
void ConfigureSystemClock(void);
#endif /* SYSCLK_H_ */

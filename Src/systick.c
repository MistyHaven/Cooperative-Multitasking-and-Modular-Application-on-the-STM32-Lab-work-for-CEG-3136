// Manage the system timer

#include "systick.h"
#include "sysclk.h"

//#define SYSTICKS 4000 // 1ms with 4MHz clock
#define SYSTICKS 48000
//48000

// SYSCLK_FREQ 48e6*0.001
static volatile Time_t sysTime = 0;

void StartSysTick() {
ConfigureSystemClock();
sysTime = 0;
SysTick->LOAD = (uint32_t) (SYSTICKS - 1); // Set reload register value
SCB->SHPR[12+SysTick_IRQn] = 7 << 5;  // Set interrupt priority
SysTick->VAL = 0;
SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
				SysTick_CTRL_TICKINT_Msk  |
				SysTick_CTRL_ENABLE_Msk;
}
// Interrupt handler
void SysTick_Handler(void){
sysTime++;
}
// Wait for system time to change
void WaitForSysTick(void) {
int wasTime = sysTime;
while (sysTime == wasTime)
// Instruction to keep CPU asleep until next interrupt
__asm volatile ("wfi");
}
// Delay measured in milliseconds
void msDelay(int t) {
for (int i = 0; i < t; i++)
WaitForSysTick ();

}

// Obtain the current system time
Time_t TimeNow(void) {
return sysTime;
}
// Calculate the elapsed system time since a previous event
Time_t TimePassed(Time_t since) {
Time_t now = sysTime;
if (now >= since)
return now - since;
// Deal with rollover
else
return now + 1 + TIME_MAX - since;
}


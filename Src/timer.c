// Timer peripheral driver
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "timer.h"
#include "gpio.h"
#include "sysclk.h"
// --------------------------------------------------------
// Initialization
// --------------------------------------------------------
// Force an update of timer registers after configuration changes
static void UpdateTimerRegisters(TIM_TypeDef *TIM) {
TIM->EGR |= TIM_EGR_UG; // Update registers
while (!(TIM->SR & 0x1)) {} // Wait for update interrupt flag
TIM->SR &= ~TIM_SR_UIF; // Clear update interrupt flag
}
// Enable the timer peripheral clock
void TimerEnable(TimerIO_t timer) {
TIM_TypeDef *TIM = timer.iface;
// Enable timer peripheral clock
if (TIM == TIM1) RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
else if (TIM == TIM8) RCC->APB2ENR |= RCC_APB2ENR_TIM8EN;
else RCC->APB1ENR1 |= 1 << (TIMER_NUM(TIM) - 2);
// Enable SYSCFG clock
RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
// Enable preload
TIM->CR1 |= TIM_CR1_ARPE;
UpdateTimerRegisters(TIM);
// Configure the GPIO pin
//...
GPIO_Enable(timer.pin);
GPIO_AltFunc(timer.pin, timer.af);
GPIO_Mode(timer.pin, ALTFUNC);
}
// Set periods for the 3 cascaded counters
void TimerPeriod(TimerIO_t tio, uint16_t psc, uint16_t arr, uint16_t rcr) {
TIM_TypeDef *TIM = tio.iface;
TIM->PSC = psc; // Prescaler register
TIM->ARR = arr; // Auto-reload register
TIM->RCR = rcr; // Repetition count register
TIM->CNT = 0; // Clear counter
UpdateTimerRegisters(TIM);
}
// Set the operating mode of a timer channel:
// Input Capture (INCAP) or Output Compare (OUTCMP)
// Timer Output: Toggle (TOG), PWM Mode 1 (PWM1), or PWM Mode 2 (PWM2)
// Timer Input: Primary (TIPRI) or Secondary (TISEC) for given channel
void TimerMode(TimerIO_t tio, TimerMode_t mode, TimerSelect_t sel) {
TIM_TypeDef *TIM = tio.iface;
volatile uint32_t *CCMR = &TIM->CCMR1 + (tio.chan - 1) / 2;
if (mode == OUTCMP) {
// Capture/compare mode register
// Select PWM Mode 1 or 2, preload enable
*CCMR |= ( (sel == PWM1 ? 0b0110 : 0b0111) << TIM_CCMR1_OC1M_Pos
| TIM_CCMR1_OC1PE ) << (tio.chan - 1) % 2 * 8;
// Enable capture/compare
TIM->CCER |= TIM_CCER_CC1E << (tio.chan - 1) * 4;
TIM->CCER |= TIM_CCER_CC2E; // Output compare mode
TIM->BDTR |= TIM_BDTR_MOE; // Main output enable
}
else if (mode == INCAP) {
// Select timer input pin T1 or T2
*CCMR |= (sel == TISEC ? 0b10 : 0b01) << TIM_CCMR1_CC1S_Pos
<< (tio.chan - 1) % 2 * 8;
// Enable capture/compare
TIM->CCER |= TIM_CCER_CC1E << (tio.chan - 1) * 4;
}
}
// Start the timer
void TimerStart(TimerIO_t tio, TimerMode_t mode) {
TIM_TypeDef *TIM = tio.iface;
// Toggle timer configuration bits
TIM->CCER &= ~(TIM_CCER_CC1E << (tio.chan - 1) * 4);
TIM->CCER |= (TIM_CCER_CC1E << (tio.chan - 1) * 4);
TIM->CR1 |= TIM_CR1_CEN; // Counter enable
}
// --------------------------------------------------------
// Observation and control
// --------------------------------------------------------
// Output Compare mode: Write CCR threshold value
void TimerOutput(TimerIO_t timer, uint16_t thresh) {
volatile uint32_t *CCR = (&timer.iface->CCR1 + timer.chan - 1);
*CCR = thresh;
}
// Input Capture mode: Read
uint16_t TimerInput(TimerIO_t timer) {
volatile uint32_t *CCR = (&timer.iface->CCR1 + timer.chan - 1);
return *CCR;
}
// --------------------------------------------------------
// Interrupt handling
// --------------------------------------------------------
// Array of callback function pointers
// Timers 1 to 8, UP + 4 CC channels for each
static void (*callbacks[8][5]) (void);
// Register a function to be called when an interrupt occurs
void TimerCallback(TimerIO_t timer, void (*func)(void), TimerFlag_t flag) {
TIM_TypeDef *TIM = timer.iface;
int i = TIMER_NUM(TIM);
int IRQn = i == 1 ? flag == UP ? TIM1_UP_IRQn : TIM1_CC_IRQn :
i == 8 ? flag == UP ? TIM8_UP_IRQn : TIM8_CC_IRQn :
TIM2_IRQn + (i - 2);
// Set callback address
callbacks[i-1][flag] = func;
// Enable interrupt generation
TIM->DIER |= (1 << flag);
UpdateTimerRegisters(TIM);
// Enable interrupt vector
NVIC->IPR[IRQn] = 0;
__COMPILER_BARRIER();
NVIC->ISER[IRQn / 32] = 1 << (IRQn % 32);
__COMPILER_BARRIER();
}
// Interrupt handler for all timers
static void TimerIRQHandler(TIM_TypeDef *TIM, int i, int IRQn) {
// Callback function pointer
void (*fp)(void);
// Clear pending IRQ
NVIC->ICPR[IRQn / 32] = 1 << (IRQn % 32);
// Detect interrupt flags
for (int j = 0; j <= 4; j++)
if (TIM->SR & 1 << j) {
fp = callbacks[i-1][j];
if (fp != NULL) {
UpdateTimerRegisters(TIM);
fp(); // Invoke callback
}
}
}
// Dispatch all Timer IRQs to common handler function
void TIM1_UP_IRQHandler() { TimerIRQHandler(TIM1, 1, TIM1_UP_IRQn); }
void TIM1_CC_IRQHandler() { TimerIRQHandler(TIM1, 1, TIM1_CC_IRQn); }
void TIM2_IRQHandler() { TimerIRQHandler(TIM2, 2, TIM2_IRQn); }
void TIM3_IRQHandler() { TimerIRQHandler(TIM3, 3, TIM3_IRQn); }
void TIM4_IRQHandler() { TimerIRQHandler(TIM4, 4, TIM4_IRQn); }
void TIM5_IRQHandler() { TimerIRQHandler(TIM5, 5, TIM5_IRQn); }
void TIM6_IRQHandler() { TimerIRQHandler(TIM6, 6, TIM6_IRQn); }
void TIM7_IRQHandler() { TimerIRQHandler(TIM7, 7, TIM7_IRQn); }
void TIM8_UP_IRQHandler() { TimerIRQHandler(TIM8, 8, TIM8_UP_IRQn); }
void TIM8_CC_IRQHandler() { TimerIRQHandler(TIM8, 8, TIM8_CC_IRQn); }


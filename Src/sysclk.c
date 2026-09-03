// System Clock (SYSCLK) configuration
#include <stdbool.h>
#include "sysclk.h"
static bool done = 0;
void ConfigureSystemClock(void) {
 if (done)
 return; // SYSCLK already configured
 // Multi-speed oscillator (MSI)
 RCC->CR |= RCC_CR_MSION | RCC_CR_MSIRDY | RCC_CR_MSIRGSEL;
 RCC->CR = (RCC->CR & ~RCC_CR_MSIRANGE_Msk) | 0x6 << RCC_CR_MSIRANGE_Pos;
 // Phase-locked loop (PLL)
 RCC->CR &= ~RCC_CR_PLLON; // Disable PLL
 RCC->PLLCFGR = 24 << RCC_PLLCFGR_PLLN_Pos // 2MHz x 24 = 48MHz
 | 0b01 << RCC_PLLCFGR_PLLSRC_Pos; // Select MSI clock input
 RCC->CR |= RCC_CR_PLLON; // Enable PLL
 RCC->PLLCFGR |= RCC_PLLCFGR_PLLREN; // Enable PLL clock output
 // Add flash read wait states to account for faster SYSCLK
 FLASH->ACR |= 0x2 << FLASH_ACR_LATENCY_Pos;
 // Select PLL as System Clock source
 RCC->CFGR |= 0x3 << RCC_CFGR_SW_Pos;
 done = true;
}

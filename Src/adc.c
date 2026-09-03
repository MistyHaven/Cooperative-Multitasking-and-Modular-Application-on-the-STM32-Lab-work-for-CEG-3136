// ADC driver
#include "adc.h"
#include "gpio.h"
void ADC_Enable (ADCInput_t ai) {
 ADC_TypeDef *ADC = ai.iface;
 // Configure GPIO pin for analog
 GPIO_Enable(ai.pin);
 GPIO_Mode(ai.pin, ANALOG);
 RCC->AHB2ENR |= RCC_AHB2ENR_ADCEN; // Enable ADC clock
 RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; // Enable SYSCFG clock
 RCC->CCIPR1 |= 1 << RCC_CCIPR1_ADCSEL_Pos; // Select PLLADC1CLK
 ADC->CR &= ~ADC_CR_ADEN; // Disable ADC
 ADC->CR &= ~ADC_CR_DEEPPWD; // Disable deep power down
 ADC12_COMMON_NS->CCR &= ~ADC_CCR_PRESC; // Clear prescaler
 ADC12_COMMON_NS->CCR |= ADC_CCR_CKMODE_1; // Set clock to HCLK/1
 // 1 conversion, selected channel, discontinuous mode
 ADC->SQR1 = 0 << ADC_SQR1_L;
 ADC->SQR1 |= (ai.chan << ADC_SQR1_SQ1_Pos);
 ADC->CFGR |= ADC_CFGR_DISCEN;
 ADC->CR &= ~(ADC_CR_JADSTP | ADC_CR_ADSTP);
 ADC->CR &= ~(ADC_CR_JADSTART | ADC_CR_ADSTART | ADC_CR_ADDIS);
 ADC->CR |= ADC_CR_ADVREGEN; // Enable ADC voltage regulator
 ADC->CR |= ADC_CR_ADEN; // Enable ADC
}
// Read ADC
uint32_t ADC_Read(ADCInput_t ai) {
 ADC_TypeDef *ADC = ai.iface;
 ADC->CR |= ADC_CR_ADSTART; // Start first ADC conversion, software triggered
 while (!(ADC->ISR & ADC_ISR_EOC)) {} // Wait for conversion to complete
 return ADC1->DR; // Conversion result
}

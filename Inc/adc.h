#ifndef ADC_H_
#define ADC_H_
#include "stm32l5xx.h"
#include "gpio.h"
// ADC input structure
typedef struct {
 ADC_TypeDef *iface; // ADC peripheral
 int chan; // Channel number
 Pin_t pin; // Input/output pin
} ADCInput_t;
void ADC_Enable(ADCInput_t ai);
uint32_t ADC_Read(ADCInput_t ai);
#endif /* ADC_H_ */

// General-purpose input/output driver
#include <stddef.h>
#include  <stdbool.h>
#include "gpio.h"
#include  "i2c.h"

//Initialization

// Enable the GPIO port peripheral clock for the specified pin
void GPIO_Enable (Pin_t pin) {
	GPIO_PortEnable(pin.port);

}

void GPIO_PortEnable  (GPIO_TypeDef *port) {
	      if(  port   == GPIOX)
	    	    I2C_Enable(LeafyI2C);
	      else
	    	  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN << GPIO_PORT_NUM(port);
}

void GPIO_Mode (Pin_t pin, PinMode_t mode) {
	pin.port->MODER &= ~(0b11 << (2 * pin.bit));     // Clear mode bits
	    pin.port->MODER |=  (mode  << (2 * pin.bit));    // Set new mode
}

void GPIO_Config (Pin_t pin, PinType_t ot, PinSpeed_t osp, PinPUPD_t pupd){  //set the pins electrical style
	  GPIO_TypeDef *GPIOx = pin.port;
	    uint32_t pos2 = (uint32_t)pin.bit * 2U;   // 2-bit fields per pin
	    uint32_t bit1 = 1UL << pin.bit;       // musk

	    // Output type (OTYPER): 0 = push-pull, 1 = open-drain
	    if ((uint32_t)ot != 0U) {                      // assume enum: 0=PP, 1=OD
	        GPIOx->OTYPER |= bit1;                      // set the pin (open drain)
	    } else {
	        GPIOx->OTYPER &= ~bit1;                    // clear the bit (push pull)
	    }

	    // Output speed (OSPEEDR): 2 bits per pin (00..11)
	    GPIOx->OSPEEDR &= ~(0x3UL << pos2);                        // clear current setting
	    GPIOx->OSPEEDR |=  ((uint32_t)osp & 0x3UL) << pos2;         // write new  speed

	    // Pull-up / Pull-down (PUPDR): 2 bits per pin (00 none, 01 PU, 10 PD)
	    GPIOx->PUPDR &= ~(0x3UL << pos2);                              // clear current setting
	    GPIOx->PUPDR |=  ((uint32_t)pupd & 0x3UL) << pos2;             // write new resistor config

}


void GPIO_AltFunc(Pin_t pin, int af){  //  hook the pin up to a special prepheral inside the chip
	GPIO_TypeDef *GPIOx = pin.port;
	    uint32_t idx   = (pin.bit < 8U) ? 0U : 1U;           // AFRL for 0..7, AFRH for 8..15
	    uint32_t shift = ((uint32_t)pin.bit & 0x7U) * 4U;    // 4 bits per pin

	    GPIOx->AFR[idx] &= ~(0xFUL << shift);                            // clear the old AF
	    GPIOx->AFR[idx] |=  ((uint32_t)af & 0xFU) << shift;              // WRITE THE NEW AF
}

// Pin observation and control

// Observe the value of an input pin
PinState_t GPIO_Input (const Pin_t pin) {
	return (pin.port->IDR & (1 << pin.bit)) ? HIGH : LOW;
}


uint16_t    GPIO_PortInput  (GPIO_TypeDef *port){
	return port ->IDR;
}

void GPIO_Output (Pin_t pin, const PinState_t state) {
          if(pin.port  ==  GPIOX){
        		if (state  ==  HIGH)
        		        pin.port->ODR   |= 1 << pin.bit;        // Set pin
        		    else
        		        pin.port->ODR   &=  ~(1 << pin.bit); // Reset pin
          }
          else {
        		if (state  ==  HIGH)
        		        pin.port->BSRR = (1 << pin.bit);        // Set pin
        		    else
        		        pin.port->BSRR = (1 << (pin.bit + 16)); // Reset pin
          }

}


void GPIO_PortOutput (GPIO_TypeDef *port, uint16_t states){
	port -> ODR = states ;
}

void GPIO_Toggle (Pin_t pin) {
	 pin.port->ODR ^=(1<<pin.bit);
}
// Interrupt handling

// Array of callback function pointers
// Bits 0 to 15 (each can select one port GPIOA to GPIOH)
// Rising and falling edge triggers for each
static void (*callbacks [16] [2]) (void) ;

// Register a function to be called when an interrupt occurs
void GPIO_Callback (Pin_t pin, void (*func) (void), PinEdge_t edge)
{
callbacks [pin.bit] [edge] = func;

// Enable interrupt generation
if (edge == RISE)
EXTI->RTSR1 |= 1 << pin.bit;
else
EXTI->FTSR1 |= 1 << pin.bit;
EXTI->EXTICR[pin.bit / 4] |= GPIO_PORT_NUM(pin.port) << 8*(pin.bit % 4);
EXTI->IMR1 |= 1 << pin.bit;

// Enable interrupt vector
NVIC->IPR[EXTI0_IRQn + pin.bit] = 0;
__COMPILER_BARRIER();
NVIC->ISER[ (EXTI0_IRQn + pin.bit) / 32] = 1 << ((EXTI0_IRQn + pin.bit) % 32);
__COMPILER_BARRIER();
}

// Interrupt handler for all GPIO pins
void GPIO_IRQHandler (int i) {
// Clear pending IRQ
NVIC->ICPR[ (EXTI0_IRQn + i) / 32] = 1 << ((EXTI0_IRQn + i) % 32);

// Detect rising edge
if (EXTI->RPR1 & (1 << i) ) {
EXTI->RPR1 = (1 << i); // Service interrupt
callbacks[i] [RISE] ();
}
// Invoke callback function

// Detect falling edge
if (EXTI->FPR1 & (1 << i) ) {
EXTI->FPR1 = (1 << i);
callbacks[i] [ FALL] () ;
}}
// Service interrupt
// Invoke callback function


// Dispatch all GPIO IRQs to common handler function
void EXTI0_IRQHandler() { GPIO_IRQHandler( 0); }
void EXTI1_IRQHandler() { GPIO_IRQHandler( 1); }
void EXTI2_IRQHandler() { GPIO_IRQHandler( 2); }
void EXTI3_IRQHandler() { GPIO_IRQHandler( 3); }
void EXTI4_IRQHandler() { GPIO_IRQHandler( 4); }
void EXTI5_IRQHandler() { GPIO_IRQHandler( 5); }
void EXTI6_IRQHandler() { GPIO_IRQHandler( 6); }
void EXTI7_IRQHandler() { GPIO_IRQHandler( 7); }
void EXTI8_IRQHandler() { GPIO_IRQHandler( 8); }
void EXTI9_IRQHandler() { GPIO_IRQHandler( 9); }
void EXTI10_IRQHandler() { GPIO_IRQHandler(10); }
void EXTI11_IRQHandler() { GPIO_IRQHandler(11); }
void EXTI12_IRQHandler() { GPIO_IRQHandler(12); }
void EXTI13_IRQHandler() { GPIO_IRQHandler(13); }
void EXTI14_IRQHandler() { GPIO_IRQHandler(14); }
void EXTI15_IRQHandler() { GPIO_IRQHandler(15); }


// Emulated GPIO registers for I/O expander
GPIO_TypeDef IOX_GPIO_Regs = {0xFFFFFFFF, 0, 0, 0, 0, 0, 0, 0, {0, 0}, 0, 0, 0};
// Transmit/receive data buffers

static uint8_t IOX_txData = 0xFF;
static uint8_t IOX_rxData = 0xFF;

// I2C transfer structures bus addr data size stop busy next

static I2C_Xfer_t IOX_LEDs = {&LeafyI2C, 0x70, &IOX_txData, 1, 1, 0, NULL};
static I2C_Xfer_t IOX_PBs = {&LeafyI2C, 0x73, &IOX_rxData, 1, 1, 0, NULL};

void UpdateIOExpanders(void) {
              // Copy to/from data buffers, with polarity inversion
             IOX_txData = ~(GPIOX->ODR & 0xFF); // LEDs in bits 7:0
             GPIOX->IDR = (~IOX_rxData) << 8; // PBs in bits 15:8
              // Keep requesting transfers to/from I/O expanders
              if (!IOX_LEDs.busy)
                      I2C_Request(&IOX_LEDs);
              if (!IOX_PBs.busy)
                     I2C_Request(&IOX_PBs);
}

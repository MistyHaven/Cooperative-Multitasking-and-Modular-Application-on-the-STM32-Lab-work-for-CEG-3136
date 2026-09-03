// I2C driver version 3
#include <stddef.h>
#include <stdio.h>
#include "i2c.h"
#include "gpio.h"
// There is one I2C bus present on the lab platform:
I2C_Bus_t LeafyI2C = {
I2C2, // I2C controller 2
{GPIOF, 0}, // SDA pin PF0
{GPIOF, 1} // SCL pin PF1
};
// Pointers to head and tail of the transfer queue
static I2C_Xfer_t *head = NULL;
static I2C_Xfer_t *tail = NULL;
static int n = -1; // Number of bytes transferred, -1 when idle
// Bit 0 of address byte indicates read vs write transfer
#define I2C_READ (head->addr & 0x1)
#define I2C_WRITE (!(head->addr & 0x1))
// Enable I2C controller and configure associated GPIO pins
void I2C_Enable (I2C_Bus_t bus) {
if (bus.iface->CR1 & I2C_CR1_PE)
return; // Already enabled
// Enable clock to selected I2C controller
RCC->APB1ENR1 |= bus.iface == I2C1 ? RCC_APB1ENR1_I2C1EN :
bus.iface == I2C2 ? RCC_APB1ENR1_I2C2EN :
bus.iface == I2C3 ? RCC_APB1ENR1_I2C3EN :
bus.iface == I2C4 ? RCC_APB1ENR2_I2C4EN : 0;
// Enable clocks to GPIO ports containing SDA and SCL pins
GPIO_Enable(bus.pinSDA);
GPIO_Enable(bus.pinSCL);
// Configure for open drain (PMOS disabled)
GPIO_Config(bus.pinSDA, OD, S0, NOPUPD);
GPIO_Config(bus.pinSCL, OD, S0, NOPUPD);
// Select alternate function as I2C
GPIO_AltFunc(bus.pinSDA, 0x4);
GPIO_AltFunc(bus.pinSCL, 0x4);
// Alternate function mode
GPIO_Mode(bus.pinSDA, ALTFUNC);
GPIO_Mode(bus.pinSCL, ALTFUNC);
// Configure I2C peripheral
bus.iface->CR1 &= ~I2C_CR1_PE;
bus.iface->TIMINGR = 0xB0420F13; //0xE14
bus.iface->CR1 = I2C_CR1_PE;
}
// Add a transfer request to the queue
void I2C_Request (I2C_Xfer_t *p) {
if (head == NULL)
head = p; // Add to empty queue
else
tail->next = p; // Add to tail of non-empty queue
tail = p;
p->next = NULL;
p->busy = true; // Mark transfer as in-progress
}
// Polling implementation, called from main loop every tick
void ServiceI2CRequests (void) {
if (head == NULL)
return; // Nothing to do right now
I2C_Xfer_t *q = head;
I2C_TypeDef *i2c = q->bus->iface;
if (n == -1) {
// Begin a new transfer
n = 0;
i2c->ICR = 0xFFFF; // Clear flags
i2c->CR2 = (q->addr & 0xFE)
| I2C_READ << I2C_CR2_RD_WRN_Pos
| q->size << I2C_CR2_NBYTES_Pos
| q->stop << I2C_CR2_AUTOEND_Pos
| I2C_CR2_START;
}
else if (n < q->size) {
if (i2c->ISR & I2C_ISR_TXIS)
// Copy transmit data from memory buffer to hardware buffer
i2c->TXDR = q->data[n++];
if (i2c->ISR & I2C_ISR_RXNE)
// Copy receive data from hardware buffer to memory buffer
q->data[n++] = i2c->RXDR;
}
else {
// Remove transfer from head of queue
head = q->next;
q->next = NULL;
q->busy = 0; // Mark transfer as complete
n = -1; // Prepare for next transfer
}
}


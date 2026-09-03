/*
 * spi.c
 *
 *  Created on: Nov 20, 2025
 *      Author: ashay020
 */


// SPI controller driver
#include <stddef.h>
#include <stdio.h>
#include "spi.h"
#include "gpio.h"
#include "systick.h"
// SPI bus for the Environmental Sensor
SPI_Bus_t EnvSPI = {
 SPI1, // SPI controller 1
 {GPIOA, 5}, // SCLK pin
 {GPIOA, 6}, // MISO pin
 {GPIOA, 7}, // MOSI pin
 {GPIOD, 14} // NSS/CSB pin
};
// Pointers to head and tail of the transfer queue
static SPI_Xfer_t *head = NULL;
static SPI_Xfer_t *tail = NULL;
static int n = -1; // Number of bytes transferred, -1 when idle
// Enable SPI controller and configure associated GPIO pins
void SPI_Enable (SPI_Bus_t bus) {
 if (bus.iface->CR1 & SPI_CR1_SPE)
 return; // Already enabled
 // Enable clock to selected SPI controller
 // See MCU Reference Manual, Table 82
 RCC-> APB2ENR |= bus.iface == SPI1 ? RCC_APB2ENR_SPI1EN : 0;
 RCC->APB1ENR1 |= bus.iface == SPI2 ? RCC_APB1ENR1_SPI2EN :
 bus.iface == SPI3 ? RCC_APB1ENR1_SPI3EN : 0;
 // Enable clocks to GPIO ports containing SPI pins
 // ...
 GPIO_Enable(bus.pinMISO); // for all of port A
 GPIO_Enable(bus.pinMOSI);
 GPIO_Enable(bus.pinSCLK);
 GPIO_Enable(bus.pinNSS); // for port D


// Alternate function mode (SCLK, MOSI, MISO only)
GPIO_Mode(bus.pinMISO, ALTFUNC);
GPIO_Mode(bus.pinSCLK, ALTFUNC);
GPIO_Mode(bus.pinMOSI, ALTFUNC);

// Select alternate function as SPI
// See MCU Datasheet, Table 22
 // ...
 GPIO_AltFunc(bus.pinSCLK, 5);
 GPIO_AltFunc(bus.pinMOSI, 5);
 GPIO_AltFunc(bus.pinMISO, 5);
 // NSS: active low GP output, default inactive
 // ...
 GPIO_Mode(bus.pinNSS, OUTPUT);
 GPIO_Output(bus.pinNSS, HIGH); // cause off is 1
 // Configure SPI peripheral
 bus.iface->CR1 &= ~SPI_CR1_SPE;
 bus.iface->CR1 = SPI_CR1_MSTR | (5<<3) | SPI_CR1_SSM | SPI_CR1_SSI;
 bus.iface->CR2 = SPI_CR2_FRXTH | (8 - 1) << SPI_CR2_DS_Pos;
 bus.iface->CR1 |= SPI_CR1_SPE;
}
// Add a transfer request to the queue
void SPI_Request (SPI_Xfer_t *p) {
 if (head == NULL)
 head = p; // Add to empty queue
 else
 tail->next = p; // Add to tail of non-empty queue
 tail = p;
 p->next = NULL;
 p->busy = true; // Mark transfer as in-progress
}
// Polling implementation, called from main loop every tick
void ServiceSPIRequests (void) {
 if (head == NULL)
 return; // Nothing to do right now
 SPI_Xfer_t *p = head;
 SPI_TypeDef *SPI = p->bus->iface;
 volatile uint8_t *DR = (volatile uint8_t *)&SPI->DR; // Workaround
 if (n == -1) {
 // Begin a new transfer
 n = 0;
 GPIO_Output(p->bus->pinNSS, LOW); // Assert select
 if (p->dir == RX) {
 *DR = 0; // Dummy transmit
 }
 }
 else if (n < p->size) {
 if (p->dir == TX && SPI->SR & SPI_SR_TXE) {
 if (n > 0)
 *DR; // Dummy receive
 // Copy transmit data from memory buffer to hardware buffer
 *DR = p->data[n++];
 }
 if (p->dir == RX && SPI->SR & SPI_SR_RXNE) {
 // Copy receive data from hardware buffer to memory buffer
 p->data[n++] = SPI->DR;
 if (n < p->size)
 *DR = 0; // Dummy transmit
 }
 }
 else {
 // Remove transfer from head of queue
 head = p->next;
 p->next = NULL;
 p->busy = 0; // Mark transfer as complete
 n = -1; // Prepare for next transfer
 if (p->dir == TX)
 // Drain the receive data buffer
 while (SPI->SR & SPI_SR_RXNE)
 *DR; // Dummy receive
 if (p->last)
 GPIO_Output(p->bus->pinNSS, HIGH); // De-assert select
 }
}

#ifndef I2C_H_
#define I2C_H_

#include <stdbool.h>
#include "stm32l5xx.h"
#include "gpio.h"

// I2C bus connection
typedef struct {
    I2C_TypeDef *iface;
    Pin_t      pinSDA;
    Pin_t      pinSCL;
}  I2C_Bus_t;

extern I2C_Bus_t LeafyI2C; // I2C bus on Leafy mainboard

// I2C transfer record
typedef struct I2C_Xfer_t {
    I2C_Bus_t    *bus;
    uint8_t       addr;
    uint8_t       *data;
    int           size;
    bool      stop;
    bool      busy;
    struct I2C_Xfer_t *next; // Pointer to next transfer in queue
} I2C_Xfer_t;

void I2C_Enable(I2C_Bus_t bus);
void I2C_Request(I2C_Xfer_t *p);

void ServiceI2CRequests(void);

#endif /* I2C_H_ */


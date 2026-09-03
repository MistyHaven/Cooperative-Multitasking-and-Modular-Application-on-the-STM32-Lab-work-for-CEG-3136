/*
 * TouchPad.c
 *
 *  Created on: Oct 30, 2025
 *      Author: ashay020
 */


// Touchpad driver
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "touchpad.h"
#include "systick.h"
#include "i2c.h"
#include "display.h"
#include "gpio.h"
static bool enabled = false;
// I2C write transfer to initialize Touchpad sensor
// Electrode Configuration Register (ECR), all 12 electrodes enabled
static uint8_t txInit[2] = {0x5E, 0x0F}; // Register Adddres, Write Data. Since we are initializing, CL and ELEPROX_EN = 0/disabled, and only ELE_EN is on. Since we want to initialize all the touch pads, we do 11xx so 0x0C
static I2C_Xfer_t PadInit = {&LeafyI2C, 0xB4, txInit, 2, 1, 0, NULL};
// I2C combined write-read transfer to read Touchpad sensor
// Touch Status Registers (lower and upper)
static uint8_t txRdAddr[1] = {0x00}; //{0x??}; // Register Address (lower)
static uint8_t rxRdData[2]; // Read Data (2 bytes)
static I2C_Xfer_t PadRdAddr = {&LeafyI2C, 0xB4, txRdAddr, 1, 0, 0, NULL};
static I2C_Xfer_t PadRdData = {&LeafyI2C, 0xB5, rxRdData, 2, 1, 0, NULL};
// Enable Touchpad driver
void TouchEnable (void) {
 if (!enabled) {
 enabled = true;
 I2C_Enable(LeafyI2C);
 I2C_Request(&PadInit);
 // Request first read
 I2C_Request(&PadRdAddr);
 I2C_Request(&PadRdData);
 }
}
static uint16_t touchData;
static bool touchCapture = false;
static Press_t lastPress;
// Single pad pressed
Press_t TouchInput (Page_t page) {
 if (page != GetPage())
 return NONE; // Hide input for inactive pages
 Press_t pad = lastPress;
 lastPress = NONE;
 return pad;
}
// Ongoing numeric entry
bool TouchEntry(Page_t page, Entry_t *num) {
 if (page != GetPage())
 return false; // Hide input for inactive pages
 bool done = false;
 if (lastPress >= N0 && lastPress <= N9)
 *num = *num * 10 + lastPress; // Add new digit
 else if (lastPress == SHIFT)
 *num /= 10; // Erase last digit
 else if (lastPress == NEXT)
 done = true; // Entry complete
 lastPress = NONE; // Empty the buffer
 return done;
}
// Called from main loop housekeeping to check for Touchpad input
void ScanTouchpad (void) {
 if (!PadRdData.busy) {
 // Process new data from Touchpad
 touchData = rxRdData[0] | rxRdData[1] << 8;
 // Rising edge detect on touchData
 if (!touchCapture && touchData != 0x0000) {
 for (Press_t n = MIN; n <= MAX; n++)
 // Check each bit for one-hot setting
 if (touchData == 1 << n) {
 // Single pad pressed
 touchCapture = true;
 lastPress = n;
 break; // Stop checking bits
 }
 }
 else if (touchData == 0x0000)
 // Nothing pressed, reset capture state
 touchCapture = false;
 // Request next read
 I2C_Request(&PadRdAddr);
 I2C_Request(&PadRdData);
 }
}
// Discard input buffer
void ClearTouchpad (void) {
 lastPress = NONE;
}

/*
 * TouchPad.h
 *
 *  Created on: Oct 30, 2025
 *      Author: ashay020
 */

#ifndef TOUCHPAD_H_
#define TOUCHPAD_H_

#include <stdint.h>
#include <stdbool.h>
#include "display.h"

// Single press on the Touchpad
typedef enum { NONE=-1, MIN=0, N0=0, M1=1, N2=2, N3=3, N4=4, N5=5, N6=6,
               N7=7, N8=8, N9=9, SHIFT=10, NEXT=11, MAX=11 } Press_t;

// Numeric entry value
typedef uint32_t Entry_t;

void TouchEnable(void);              // Initialize touch sensor
Press_t TouchInput(Page_t page);     // Get single pad press
bool TouchEntry(Page_t page, Entry_t *num);  // Ongoing numeric entry

void ScanTouchpad(void);             // Housekeeping: Check for Touchpad input
void ClearTouchpad(void);            // Discard input buffer

#endif // TOUCHPAD_H_

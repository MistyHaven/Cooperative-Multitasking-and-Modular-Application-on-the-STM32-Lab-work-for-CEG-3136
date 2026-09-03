// Display driver version 3
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include "display.h"
#include "i2c.h"
#include "systick.h"
#include "touchpad.h"
bool enabled = false; // Initialization complete
Page_t openPage = 0; // Currently displayed page
static const Pin_t TouchEn = {GPIOB, 5}; // Pin PB5 <- Touch En button
Time_t pressTime; // Timestamp of last button press
#define DEBOUNCE_TIME 50 // 50ms debounce
static void CallbackTouchEnPress(void);
static void CallbackTouchEnRelease(void);
// --------------------------------------------------------
// Display controller
// --------------------------------------------------------
#define ROWS 2 // Number of rows
#define COLS 16 // Number of columns
uint8_t dispText[PAGES][ROWS][COLS+1];
// Command word
typedef struct {
 uint8_t ctrl; // Control byte
uint8_t data; // Data byte
} DispCmd_t;
// Initialization command sequence
static const DispCmd_t txInit[] = {
 {0x80, 0x28}, // Function Set: 2-line, display OFF
 {0x80, 0x0C}, // Display Control: display ON, cursor OFF, blink OFF
 {0x80, 0x01}, // Display Clear
 {0x80, 0x06} // Entry Mode Set: increment, no shift
};
// Display line
typedef struct {
 DispCmd_t cmd; // Command word to set display line
 uint8_t ctrl; // Last control byte, data bytes to follow
 uint8_t text[COLS]; // ASCII text to write to display
} DispLine_t;
// Select display line and print text
static DispLine_t txLine[ROWS] = {
 { {0x80, 0x80}, 0x40, {0} },
 { {0x80, 0xC0}, 0x40, {0} } };
static bool updateLine[2] = {false, false};
// I2C transfers
static I2C_Xfer_t DispInit = {&LeafyI2C, 0x7C, (uint8_t *)&txInit, 8, 1};
static I2C_Xfer_t DispLine[ROWS] = {
 {&LeafyI2C, 0x7C, (uint8_t *)&txLine[0], 19, 1},
 {&LeafyI2C, 0x7C, (uint8_t *)&txLine[1], 19, 1} };
// Enable LCD display
void DisplayEnable (void) {
 if (!enabled) {
 enabled = true;
 I2C_Enable(LeafyI2C);
 I2C_Request(&DispInit);
 // Blank out display text on all pages/rows
 for (int i = 0; i < PAGES ; i++)
 for (int j = 0; j < ROWS; j++)
 for (int k = 0; k < COLS ; k++)
 dispText[i][j][k] = ' ';
 // Use the Touch En button to cycle between display pages
 GPIO_Enable(TouchEn);
 GPIO_Mode(TouchEn, INPUT);
 GPIO_Callback(TouchEn, CallbackTouchEnPress, RISE);
 GPIO_Callback(TouchEn, CallbackTouchEnRelease, FALL);
 }
}
// Print a line of text with optional format specifiers
void DisplayPrint (Page_t page, const int line, const char *msg, ...) {
 va_list args;
 va_start(args, msg);
 // Full buffer with formatted text and space pad the remainder
 int chars = vsnprintf((char *)dispText[page][line], COLS+1, msg, args);
 for (int i = chars; i < COLS; i++)
 dispText[page][line][i] = ' ';
 if (page == openPage)
 updateLine[line] = true;
}
// --------------------------------------------------------
// Backlight controller
// --------------------------------------------------------
Color_t dispColor[PAGES] = {OFF, CYAN, MAGENTA, ORANGE};
typedef struct {
 uint8_t addr; // Address byte
 uint8_t data; // Data byte
} BltCmd_t;
// Transmit data buffers to set brightness of each LED, all off by default
static BltCmd_t txRed = {0x01, 0x00};
static BltCmd_t txGreen = {0x02, 0x00};
static BltCmd_t txBlue = {0x03, 0x00};
static bool updateBlt = true;
// I2C transfers
static I2C_Xfer_t BltRed = {&LeafyI2C, 0x5A, (void *)&txRed, 2, 1};
static I2C_Xfer_t BltGreen = {&LeafyI2C, 0x5A, (void *)&txGreen, 2, 1};
static I2C_Xfer_t BltBlue = {&LeafyI2C, 0x5A, (void *)&txBlue, 2, 1};
// Set new backlight color
void DisplayColor(const Page_t page, const Color_t color) {
 dispColor[page] = color;
 if (page == openPage)
 updateBlt = true;
}
// --------------------------------------------------------
// Automatic background updates
// --------------------------------------------------------
// Called from main loop
void UpdateDisplay(void) {
 // Update display text
 for (int j = 0; j < ROWS; j++)
 if (!DispLine[j].busy && updateLine[j]) {
 updateLine[j] = false;
 // Copy text into buffer
 for (int k = 0; k < COLS; k++)
 txLine[j].text[k] = dispText[openPage][j][k];
 I2C_Request(&DispLine[j]);
 }
 // Update backlight
 if (!BltBlue.busy && updateBlt) {
 updateBlt = false;
 // Extract individual color bytes
 Color_t color = dispColor[openPage];
 txRed .data = (color >> 16) & 0xFF;
 txGreen.data = (color >> 8) & 0xFF;
 txBlue .data = (color >> 0) & 0xFF;
 I2C_Request(&BltRed);
 I2C_Request(&BltGreen);
 I2C_Request(&BltBlue);
 }
}
// --------------------------------------------------------
// Page switching
// --------------------------------------------------------
// Obtain the currently displayed page
Page_t GetPage (void) {
 return openPage;
}
static void CallbackTouchEnPress (void) {
 pressTime = TimeNow();
}
static void CallbackTouchEnRelease (void) {
 Time_t heldTime = TimePassed(pressTime);
 if (heldTime > DEBOUNCE_TIME) {
 // Switch to next page
 openPage++;
 openPage %= PAGES;
 // Force update of display text and backlight color
 for (int i = 0; i < ROWS; i++)
 updateLine[i] = true;
 updateBlt = true;
 ClearTouchpad(); // Discard input buffer
 }
}

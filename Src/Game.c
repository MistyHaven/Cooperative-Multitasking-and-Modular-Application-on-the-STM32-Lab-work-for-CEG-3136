// Src/game.c
#include <stdlib.h>
#include <stdint.h>
#include "game.h"
#include "gpio.h"
#include "systick.h"
#include "display.h"

// Game Parameters
#define LED_COUNT        8
#define SHOW_ON_MS       800   // LEDs turned on during the show seq
#define SHOW_OFF_MS      200   // LEDs turned off between two flashes
#define MAX_SEQ_LEN      64    // max sequence length

static const Pin_t StartButton  = {GPIOX, 11}; // SW4 (start)

// --------------------- Game States  -----------------------
typedef enum {
    ST_TITLE = 0,     // title screen
    ST_SHOW_SEQ,      // show seq
    ST_WAIT_INPUT,    // wait for user's input
    ST_GAME_OVER      // round done
} GameState_t;

static GameState_t state;

// Sequences and indexes
static uint8_t  seq[MAX_SEQ_LEN];
static int      seq_len;
static int      input_idx;

// non-blocking Timers (better than time delay, mini-clock in the back)
static Time_t tBlink;
static Time_t tShow;
static uint8_t show_phase_on;
static int      show_idx;

// LEDs and buttons
static uint8_t  led_mask;
static uint16_t prevButtons;

// --------------------- Helpers GPIO -----------------------
static inline uint8_t readButtons8(void) {
    // Buttons on bits 15..8, active at 0 (active-low) → on inverse.
    return (uint8_t)((~(GPIO_PortInput(GPIOX) >> 8)) & 0xFF);
}
static inline void setLEDs(uint8_t mask) {
    led_mask = mask & 0xFF;
    GPIO_PortOutput(GPIOX, led_mask);
}

// --------------------- Screen Interface --------------------
static void ui_title(void) {
    DisplayColor(ALARM, CYAN);
    DisplayPrint(ALARM, 0, "MEMORY");
    DisplayPrint(ALARM, 1, "Press START");
}
static void ui_round(void) {
    DisplayColor(ALARM, GREEN);
    DisplayPrint(ALARM, 0, "Round: %d", seq_len);
    DisplayPrint(ALARM, 1, "Watch...");
}
static void ui_input(void) {
    DisplayColor(ALARM, WHITE);
    DisplayPrint(ALARM, 0, "Repeat!");
    DisplayPrint(ALARM, 1, "Step %d/%d", input_idx + 1, seq_len);
}
static void ui_gameover(int best_len) {
    DisplayColor(ALARM, YELLOW);
    DisplayPrint(ALARM, 0, "Game Over");
    DisplayPrint(ALARM, 1, "Length: %d", best_len);
}

// --------------------- Init -------------------------------
void Init_Game(void) {
    GPIO_PortEnable(GPIOX);
    setLEDs(0);
    prevButtons   = 0;
    state         = ST_TITLE;
    show_phase_on = 0;
    tBlink        = TimeNow();
    ui_title();
}

// --------------------- Principle Loop ---------------------
void Task_Game(void) {
    const Time_t now   = TimeNow();
    const uint8_t btns = readButtons8();
    const uint16_t rising = (uint16_t)(btns & ~prevButtons);
    prevButtons = btns;

    switch (state) {

    // ---------- Title ----------
    case ST_TITLE: {
        // Random light blinks 800/200 ms
        if (show_phase_on) {
            if (TimePassed(tBlink) >= SHOW_ON_MS) {
                setLEDs(0);
                show_phase_on = 0;
                tBlink = now;
            }
        } else {
            if (TimePassed(tBlink) >= SHOW_OFF_MS) {
                uint8_t rnd = (uint8_t)(1u << (rand() % LED_COUNT));
                setLEDs(rnd);
                show_phase_on = 1;
                tBlink = now;
            }
        }

        // Start => new round
        if (rising & (1u << (StartButton.bit - 8))) {
            srand((unsigned)now);
            seq_len        = 1;
            seq[0]         = (uint8_t)(rand() % LED_COUNT);
            show_idx       = 0;
            show_phase_on  = 0;
            tShow          = now;
            setLEDs(0);
            ui_round();
            state = ST_SHOW_SEQ;
        }
    } break;

    // ---------- Display sequence ----------
    case ST_SHOW_SEQ: {
        if (!show_phase_on) {
            if (TimePassed(tShow) >= SHOW_OFF_MS) {
                setLEDs((uint8_t)(1u << seq[show_idx]));
                show_phase_on = 1;
                tShow = now;
            }
        } else {
            if (TimePassed(tShow) >= SHOW_ON_MS) {
                setLEDs(0);
                show_phase_on = 0;
                tShow = now;
                show_idx++;
                if (show_idx >= seq_len) {
                    input_idx = 0;
                    ui_input();
                    state = ST_WAIT_INPUT;
                }
            }
        }
    } break;

    // ---------- Player's input -----------
    case ST_WAIT_INPUT: {
        // Visual Feedback from buttons
        setLEDs(btns);

        if (rising) {
            for (int i = 0; i < LED_COUNT; i++) {
                if (rising & (1u << i)) {
                    if (i == seq[input_idx]) {
                        input_idx++;
                        DisplayPrint(ALARM, 1, "Step %d/%d", input_idx, seq_len);
                        if (input_idx >= seq_len) {
                            if (seq_len < MAX_SEQ_LEN)
                                seq[seq_len++] = (uint8_t)(rand() % LED_COUNT);
                            show_idx = 0;
                            show_phase_on = 0;
                            tShow = now;
                            setLEDs(0);
                            ui_round();
                            state = ST_SHOW_SEQ;
                        }
                    } else {
                        setLEDs((uint8_t)(seq_len - 1));   // previous length in binary
                        ui_gameover(seq_len - 1);
                        state = ST_GAME_OVER;
                    }
                    break;
                }
            }
        }
    } break;

    // ---------- End of round ----------
    case ST_GAME_OVER: {
        if (rising) {
            setLEDs(0);
            ui_title();
            state = ST_TITLE;
        }
    } break;

    } // switch
}

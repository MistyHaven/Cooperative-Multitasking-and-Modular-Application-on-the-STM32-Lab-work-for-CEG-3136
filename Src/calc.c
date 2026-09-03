/*
* calc.c
*/
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "calc.h"
#include "display.h"
#include "touchpad.h"

#define NMAX 12

static Press_t operation;
static Entry_t operand[NMAX];
static int     count;
static Entry_t result;
static enum {MENU, PROMPT, ENTRY, RUN, SHOW, WAIT} state;

/* extras */
static uint32_t op4   = 0;      // 1:+ 2:- 3:* 4:/
static uint32_t listN = 0;
static int      menuPage = 0;

/* sort display paging */
static char     sortBuf[128];
static uint32_t sortLen = 0;
static uint32_t sortOff = 0;

/* asm functions */
uint32_t Increment(uint32_t n);
uint32_t Decrement(uint32_t n);
uint32_t FourOp(uint32_t op, uint32_t a, uint32_t b);
uint32_t Gcd   (uint32_t a, uint32_t b);
uint32_t Fact  (uint32_t n);
uint32_t Fib   (uint32_t n);
uint32_t Sort  (uint32_t* p, uint32_t n);
uint32_t Avg1dp(uint32_t* p, uint32_t n);

static void RenderMenu(void) {
    if (menuPage == 0) {
        DisplayPrint(CALC, 0, "Calculator app");
        DisplayPrint(CALC, 1, "1:INC 2:DEC 3:4F");
    } else {
        DisplayPrint(CALC, 0, "4:GCD 5:FAC 6:FB");
        DisplayPrint(CALC, 1, "7:SORT 8:AVG");
    }
}

void Init_Calc (void) {
    DisplayEnable();
    TouchEnable();
    state = MENU;
}

void Task_Calc (void) {
    switch (state) {

    case MENU:
        operation = NONE;
        for (int i=0;i<NMAX;i++) operand[i]=0;
        count=0; result=0; op4=0; listN=0;
        sortBuf[0]='\0'; sortLen=0; sortOff=0;

        menuPage = 0;
        RenderMenu();
        state = PROMPT;
        break;

    case PROMPT: {
        if (operation == NONE) {
            Press_t key = TouchInput(CALC);
            if (key == NEXT) {               // toggle menu pages
                menuPage ^= 1;
                RenderMenu();
                break;
            }
            operation = key;
        }

        switch ((int)operation) {
            case 1: case 2:   // INC / DEC
                if (count == 1) state = RUN;
                else { DisplayPrint(CALC,0,"Enter a number:"); state = ENTRY; }
                break;

            case 3: // 4-function
                if (op4 == 0) {
                    DisplayPrint(CALC,0,"4-Function:");
                    DisplayPrint(CALC,1,"1:+ 2:- 3:* 4:/");
                    {
                        Press_t pick = TouchInput(CALC);
                        if (pick>=1 && pick<=4) op4=(uint32_t)pick;
                    }
                    break;
                } else if (count < 1) {
                    DisplayPrint(CALC,0,"Enter A:");
                    state = ENTRY;
                } else if (count < 2) {
                    DisplayPrint(CALC,0,"Enter B:");
                    state = ENTRY;
                } else {
                    state = RUN;
                }
                break;

            case 4: // GCD
                if (count==0)      { DisplayPrint(CALC,0,"Enter A:"); state=ENTRY; }
                else if (count==1) { DisplayPrint(CALC,0,"Enter B:"); state=ENTRY; }
                else                state = RUN;
                break;

            case 5: // FACT
            case 6: // FIB
                if (count==1) state = RUN;
                else { DisplayPrint(CALC,0,"Enter n:"); state = ENTRY; }
                break;

            case 7: // SORT
            case 8: // AVG
                if (listN==0) {
                    DisplayPrint(CALC,0,"How many (<=10)?");
                    state = ENTRY;
                } else if ((uint32_t)(count-1) < listN) {
                    DisplayPrint(CALC,0,"Enter x[%u]:",(unsigned)count);
                    state = ENTRY;
                } else {
                    state = RUN;
                }
                break;

            default:
                operation = NONE;
                break;
        }
        break;
    }

    case ENTRY: {
        bool done = TouchEntry(CALC, &operand[count]);
        DisplayPrint(CALC, 1, "%u", operand[count]);
        if (done) {
            count++;
            if ((operation==7 || operation==8) && listN==0) {
                listN = (operand[0] > 10) ? 10 : (uint32_t)operand[0];
            }
            state = PROMPT;
        }
        break;
    }

    case RUN:
        DisplayPrint(CALC,0,"Calculating...");
        DisplayPrint(CALC,1,"");
        state = SHOW;

        switch ((int)operation) {
            case 1: result = Increment(operand[0]); break;
            case 2: result = Decrement(operand[0]); break;

            case 3: result = FourOp(op4, operand[0], operand[1]); break;

            case 4: result = Gcd(operand[0], operand[1]); break;

            case 5: result = Fact(operand[0]); break;

            case 6: result = Fib(operand[0]); break;

            case 7: { // SORT -> build comma-separated string and enable paging
                uint32_t n = listN; if (n>10) n=10;
                Sort((uint32_t*)&operand[1], n);
                // build "a,b,c,..." into sortBuf
                char* p = sortBuf; size_t rem = sizeof(sortBuf);
                p[0]='\0';
                for (uint32_t i=0;i<n;i++) {
                    int w = snprintf(p, rem, (i==0) ? "%u" : ",%u", (unsigned)operand[1+i]);
                    if (w < 0 || (size_t)w >= rem) break;
                    p += w; rem -= (size_t)w;
                }
                sortLen = (uint32_t)strlen(sortBuf);
                sortOff = 0;
                // first screen
                DisplayPrint(CALC,0,"Sorted:");
                DisplayPrint(CALC,1,"%.16s", sortBuf); // first 16 chars
                state = WAIT;
                return; // handled fully in WAIT paging
            }

            case 8: { // AVG
                uint32_t n = listN; if (n>10) n=10;
                uint32_t avg10 = Avg1dp((uint32_t*)&operand[1], n);
                DisplayPrint(CALC,0,"Average:");
                DisplayPrint(CALC,1,"%u.%u",(unsigned)(avg10/10),(unsigned)(avg10%10));
                state = WAIT;
                return;
            }

            default: break;
        }
        break;

    case SHOW:
        DisplayPrint(CALC,0,"Result:");
        DisplayPrint(CALC,1,"%u", result);
        state = WAIT;
        break;

    case WAIT: {
        Press_t key = TouchInput(CALC);
        if (operation == 7 && sortLen > 16) {
            // paging for sort list
            if (key == NEXT) {
                sortOff += 16;
                if (sortOff >= sortLen) {
                    // finished; next press returns to menu
                    sortOff = 0;
                    state = MENU;
                } else {
                    DisplayPrint(CALC,0,"Sorted:");
                    // show next chunk
                    char seg[17]; seg[16]='\0';
                    for (int i=0;i<16;i++) {
                        uint32_t idx = sortOff + i;
                        seg[i] = (idx < sortLen) ? sortBuf[idx] : ' ';
                    }
                    DisplayPrint(CALC,1,"%s", seg);
                }
                break;
            }
        }
        if (key != NONE) state = MENU;
        break;
    }
    }
}



/*
 * maths.s
 *
 *  Created on: Oct 30, 2025
 *      Author: ashay020
 */

// Math routines for Calculator app
.syntax unified
.cpu cortex-m33
.thumb

.section .text

// ---------------------------------------------------------------------
// uint32_t Increment(uint32_t num);
.global Increment
.type   Increment, %function
Increment:
    adds    r0, r0, #1
    bx      lr

// uint32_t Decrement(uint32_t num);
.global Decrement
.type   Decrement, %function
Decrement:
    subs    r0, r0, #1
    bx      lr

// ---------------------------------------------------------------------
// uint32_t FourOp(uint32_t op, uint32_t a, uint32_t b);
// op: 1=Add, 2=Sub, 3=Mul, 4=Div (unsigned; /0 => 0)
.global FourOp
.type   FourOp, %function
FourOp:
    push    {lr}
    cmp     r0, #1
    beq     1f
    cmp     r0, #2
    beq     2f
    cmp     r0, #3
    beq     3f
    cmp     r0, #4
    beq     4f
    movs    r0, #0
    b       9f
1:  adds    r0, r1, r2
    b       9f
2:  subs    r0, r1, r2
    b       9f
3:  mul    r0, r1, r2
    b       9f
4:  cmp     r2, #0
    beq     8f
#ifdef __ARM_FEATURE_IDIV
    udiv    r0, r1, r2
#else
    // portable slow path (OK for lab ranges)
    movs    r0, #0              // q=0
7:  cmp     r1, r2
    blt     9f
    subs    r1, r1, r2
    adds    r0, r0, #1
    b       7b
#endif
    b       9f
8:  movs    r0, #0
9:  pop     {pc}

// ---------------------------------------------------------------------
// uint32_t Gcd(uint32_t a, uint32_t b);  // Euclid (a%b)
.global Gcd
.type   Gcd, %function
Gcd:
    push    {lr}
0:  cmp     r1, #0
    beq     3f
    // r2 = a % b  (slow modulus by repeated subtraction)
    mov     r2, r0
1:  cmp     r2, r1
    blt     2f
    subs    r2, r2, r1
    b       1b
2:  mov     r0, r1
    mov     r1, r2
    b       0b
3:  pop     {pc}

// ---------------------------------------------------------------------
// uint32_t Fact(uint32_t n); // iterative
.global Fact
.type   Fact, %function
Fact:
    push    {lr}
    movs    r1, #1              // acc = 1
    cmp     r0, #0
    beq     2f
1:  muls    r1, r1, r0
    subs    r0, r0, #1
    bne     1b
2:  mov     r0, r1
    pop     {pc}


// -------------------- Fib (iterative, robust) -----
// uint32_t Fib(uint32_t n);
.global Fib
.type   Fib, %function
Fib:
    /* Iterative:
       if n==0 return 0
       a=0; b=1;
       repeat n times: t=a+b; a=b; b=t;
       return a;
    */
    push    {r4, lr}
    cmp     r0, #0

    beq     Fib_ret0

    movs    r1, #0      /* a */
    movs    r2, #1      /* b */
Fib_loop:
    /* t = a + b */
    adds    r3, r1, r2
    /* a = b; b = t; */
    mov     r1, r2
    mov     r2, r3
    subs    r0, r0, #1
    bne     Fib_loop
    /* return a */
    mov     r0, r1
    pop     {r4, pc}

Fib_ret0:
    movs    r0, #0
    pop     {r4, pc}

// -------------------- Sort (bubble) ---------------
.global Sort
.type   Sort, %function
Sort:
    push    {r4-r7, lr}
    cmp     r1, #1
    bls     9f
    mov     r4, r0
    mov     r5, r1
Outer:
    subs    r5, r5, #1
    movs    r6, #0
    movs    r7, #0
Inner:
    cmp     r7, r5
    bge     CheckDone
    lsls    r2, r7, #2
    add     r2, r4, r2
    ldr     r3,  [r2]
    ldr     r12, [r2, #4]
    cmp     r3, r12
    bls     NoSwap
    str     r12, [r2]
    str     r3,  [r2, #4]
    movs    r6, #1
NoSwap:
    adds    r7, r7, #1
    b       Inner
CheckDone:
    cmp     r6, #0
    bne     Outer
9:  movs    r0, #0
    pop     {r4-r7, pc}



// ---------------------------------------------------------------------
// uint32_t Avg1dp(uint32_t* p, uint32_t n);  // return (sum*10)/n
.global Avg1dp
.type   Avg1dp, %function
Avg1dp:
    push    {r4-r7, lr}
    movs    r2, #0
    cmp     r1, #0
    beq     ZeroN
    mov     r4, r0          // base
    mov     r5, r1          // n
    movs    r6, #0
SumLoop:
    lsls    r3, r6, #2
    ldr     r7, [r4, r3]
    adds    r2, r2, r7
    adds    r6, r6, #1
    cmp     r6, r5
    blt     SumLoop
    movs    r3, #10
    mul    r2, r2, r3      // sum *= 10
#ifdef __ARM_FEATURE_IDIV
    udiv    r0, r2, r1
#else
    movs    r0, #0
DivLp:
    cmp     r2, r1
    blt     DivDone
    subs    r2, r2, r1
    adds    r0, r0, #1
    b       DivLp
DivDone:
#endif
    pop     {r4-r7, pc}
ZeroN:
    movs    r0, #0
    pop     {r4-r7, pc}


#include "osKernel.h"

#define PERIODICT_TASK_SCHEDULE   // Enable periodic task scheduling

#define NUM_OF_THREADS  3
#define STACKSIZE       128

#define BUS_FREQ      16000000U

#define STK_ENABLE          (1U << 0)
#define STK_TICKINT         (1U << 1)
#define STK_CLKSOURCE       (1U << 2)
#define STK_COUNTFLAG       (1U << 16)
#define STK_RESET           0x00000000U

#define THUMB_BIT           24U

#define INT_CTRL_REG        (*((volatile uint32_t*)0xE000ED04))
#define PENDSTSET           (1U << 26)


// uint32_t MILLIS_PRESCALER = BUS_FREQ / 1000U;
#define MILLIS_PRESCALER  (BUS_FREQ / 1000U)

struct tcb{
    int32_t *stackPt;          // Pointer to stack
    struct tcb *nextPt;        // Pointer to next TCB
};

typedef struct tcb tcbType;

tcbType tcbs[NUM_OF_THREADS];
volatile tcbType *currentPt;

#define PERIOD  100
uint32_t timer_periodict = 0;

int32_t TCB_STACK[NUM_OF_THREADS][STACKSIZE];

void osKernalStackInit(uint8_t i) {
    /* init SP point to top of stack */
    tcbs[i].stackPt = &TCB_STACK[i][STACKSIZE - 16]; // stack pointer

    TCB_STACK[i][STACKSIZE - 1] = (1U << THUMB_BIT); // xPSR in thumb mode

    /* dummy value in stack */
        TCB_STACK[i][STACKSIZE - 3]     = 0xAAAAAAAA; // LR
        TCB_STACK[i][STACKSIZE - 4]     = 0xAAAAAAAA; // R12
        TCB_STACK[i][STACKSIZE - 5]     = 0xAAAAAAAA; // R3
        TCB_STACK[i][STACKSIZE - 6]     = 0xAAAAAAAA; // R2
        TCB_STACK[i][STACKSIZE - 7]     = 0xAAAAAAAA; // R1
        TCB_STACK[i][STACKSIZE - 8]     = 0xAAAAAAAA; // R0
        TCB_STACK[i][STACKSIZE - 9]     = 0xAAAAAAAA; // R11
        TCB_STACK[i][STACKSIZE - 10]    = 0xAAAAAAAA; // R10
        TCB_STACK[i][STACKSIZE - 11]    = 0xAAAAAAAA; // R9
        TCB_STACK[i][STACKSIZE - 12]    = 0xAAAAAAAA; // R8
        TCB_STACK[i][STACKSIZE - 13]    = 0xAAAAAAAA; // R7
        TCB_STACK[i][STACKSIZE - 14]    = 0xAAAAAAAA; // R6
        TCB_STACK[i][STACKSIZE - 15]    = 0xAAAAAAAA; // R5
        TCB_STACK[i][STACKSIZE - 16]    = 0xAAAAAAAA; // R4
}

uint8_t osKernelAddThreads(void (*task0)(void),
                           void (*task1)(void),
                           void (*task2)(void)) {

    /* disable global interrupts */
    __disable_irq();

    /* link tcbs */
    tcbs[0].nextPt = &tcbs[1];
    tcbs[1].nextPt = &tcbs[2];
    tcbs[2].nextPt = &tcbs[0];

    /* Init stack for thread0/task0 */
    osKernalStackInit(0);
        /* Init PC */
    TCB_STACK[0][STACKSIZE - 2] = (int32_t)(task0);

    /* Init stack for thread0/task1 */
    osKernalStackInit(1);
        /* Init PC */
    TCB_STACK[1][STACKSIZE - 2] = (int32_t)(task1);

    /* Init stack for thread0/task2 */
    osKernalStackInit(2);
        /* Init PC */
    TCB_STACK[2][STACKSIZE - 2] = (int32_t)(task2);

    /* set currentPt to point to the first thread */
    currentPt = &tcbs[0];

    /* enable global interrupts */
    __enable_irq();

    return 1;
}

void osKernelLaunch(uint32_t quanta) {
    /* reset systick */
    SysTick->CTRL = STK_RESET;
    
    /* clear current systick value */
    SysTick->VAL = 0;

    /* load quanta */
    SysTick->LOAD = (quanta * MILLIS_PRESCALER) - 1;

    /* set priority to lowest */
    NVIC_SetPriority(SysTick_IRQn, 15);

    /*enable systick, using internal clock*/
    SysTick->CTRL = STK_CLKSOURCE | STK_ENABLE;

    /* enable systick interrupt */
    SysTick->CTRL |= STK_TICKINT; // enable systick with core clock and interrupts

    /* start first task */
    osSchedulerLaunch();
}

__attribute__((naked)) void SysTick_Handler(void) {
    /* SUPPEND CURRENT THREAD */

    /* disable global interrupt */
    __asm("CPSID I");

    /* saving r4 -> r11 */
    __asm("PUSH {R4-R11}");

    /* load the current stack address to R0 */
    __asm("LDR R0, =currentPt");

    /* take memory in R0 and load it to R1, now R1 points to current TCB */
    __asm("LDR R1, [R0]");

    /* store SP to currentPt->stackPt, save SP to tcb */
    __asm("STR SP, [R1]");

    /* SWITCH TO NEXT THREAD */
#ifndef PERIODICT_TASK_SCHEDULE
    /* get the next TCB by +4 bytes, defined in struct.
    * load address of next TCB to R1 */
    __asm("LDR R1, [R1, #4]"); // currentPt = currentPt->nextPt

    /* store R1 in the value that R0 point to which is currentPt, i.e currentPt = r1 (current TCB) */
    __asm("STR R1, [R0]");

    /* load CPU SP from address which R1 point to */
    __asm("LDR SP, [R1]");
#else
    /* save context of R0 and LR*/
    __asm("PUSH {R0, LR}");
    /* go to function */
    __asm("BL osShedulerRRPeriodicTask");
    /* resume R0 and LR*/
    __asm("POP {R0, LR}");

    /* load currentPt now at R0*/
    __asm("LDR R1, [R0]");

    /* load current stack in SP*/
    __asm("LDR SP, [R1]");
#endif
    /* restore r4->r11 */
    __asm("POP {R4-R11}");

    /* enable global interrupt */
    __asm("CPSIE I");

    /* return from interrupt */
    __asm("BX LR");
}



/*TODO: check the order of POP LR, might take garbage value in LR*/
void osSchedulerLaunch(void) {
    /* Load address of currentPT in R0*/
    __asm("LDR R0, =currentPt");

    /* Load in R2 value in R0*/
    __asm("LDR R2, [R0]");

    /* Load SP from address in R2, SP = currentPT->stackPt*/
    __asm("LDR SP, [R2]");

    /* restore R4 -> R11*/
    __asm("POP {R4-R11}");

    /* restore R12*/
    __asm("POP {R12}");

    /* restore R0 -> R3*/
    __asm("POP {R0-R3}");

    /* skip LR */
    __asm("ADD SP, SP, #4");

    /*create new start location by poping LR*/
    __asm("POP {LR}");

    /* skiping PSR*/
    __asm("ADD SP, SP, #4");

    /* enable global interrupt */
    __asm("CPSIE I");

    /* return from exception */
    __asm("BX LR");

}

void osThreadYeild(void) {
    /* reset systick value */
    SysTick->VAL = 0;

    /*trigger systick*/
    INT_CTRL_REG |= PENDSTSET;
}

void osShedulerRRPeriodicTask(void) {
    timer_periodict++;
    if (timer_periodict >= PERIOD) {
        (*task3)();
        timer_periodict = 0;
    }
    /* continues to next thread in RR already init thread 0->2*/
    currentPt = currentPt->nextPt;
} 


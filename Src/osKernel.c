#include "osKernal.h"

#define NUM_OF_THREADS  3
#define STACKSIZE       100

#define BUS_FREQ      16000000U

#define STK_ENABLE          (1U << 0)
#define STK_TICKINT         (1U << 1)
#define STK_CLKSOURCE       (1U << 2)
#define STK_COUNTFLAG       (1U << 16)
#define STK_RESET           0x00000000U

uint32_t MILLIS_PRESCALER = BUS_FREQ / 1000U;

struct tcb{
    int32_t *stackPt;          // Pointer to stack
    struct tcb *nextPt;        // Pointer to next TCB
};

typedef struct tcb tcbType;

tcbType tcbs[NUM_OF_THREADS];
tcbType *currentPt;

int32_t TCB_STACK[NUM_OF_THREADS][STACKSIZE];

void osKernalStackInit(uint8_t i) {
    /* init SP point to top of stack */
    tcbs[i].stackPt = &TCB_STACK[i][STACKSIZE - 16]; // stack pointer

    TCB_STACK[0][STACKSIZE - 1] = (1U << 21); // xPSR in thumb mode

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

uint8_t osKernelAddThreads((void)(*task0)(void),
                           (void)(*task1)(void),
                           (void)(*task2)(void)) {

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
    osKernalStartFirstThread();
}

__attribute__((naked)) void SysTick_Handler(void) {
    /* SUPPEND CURRENT THREAD */

    /* disable global interrupt */
    __asm("CPSID I");

    /* saving r4 -> r11 */
    __asm("PUSH {R4-R11}");

    /* save stackPt to currentPt->stackPt */
    __asm("LDR R0, =currentPt");

    /* load address of currentPt */
    __asm("LDR R1, [R0]");

    /* store SP to currentPt->stackPt, save SP to tcb */
    __asm("STR SP, [R1]");

    /* SWITCH TO NEXT THREAD */
    /* load r1 from 4bytes above r1 */
    __asm("LDR R1, [R1, #4]"); // currentPt = currentPt->nextPt

    /* store r1 at addess equals r0, i.e currentPt = r1 */
    __asm("STR R1, [R0]");

    /* load SP from address store in r1 */
    __asm("LDR SP, [R1]");

    /* restore r4->r11 */
    __asm("POP {R4-R11}");

    /* enable global interrupt */
    __asm("CPSIE I");

    /* return from interrupt */
    __asm("BX LR");
}
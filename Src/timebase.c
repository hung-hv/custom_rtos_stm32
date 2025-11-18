#include "timebase.h"
#include "stm32f4xx.h"

#define ONE_SEC_LOAD    16000000U
#define ONE_MSEC_LOAD       16000U      // 16MHz / 1000 = 16000 for 1ms tick


#define STK_ENABLE          (1U << 0)
#define STK_TICKINT         (1U << 1)
#define STK_CLKSOURCE       (1U << 2)
#define STK_COUNTFLAG       (1U << 16)

#define MAX_DELAY           0xFFFFFFFFU

volatile uint32_t g_curr_tick;
volatile uint32_t g_curr_tick_p;

#define TICK_FREQ_MS        1U

void delay(uint32_t delay) {
    uint32_t tick_start = get_tick();
    uint32_t wait = delay;

    if (wait < MAX_DELAY) {
        wait += TICK_FREQ_MS;
    }

    while( (get_tick()-tick_start) < wait ) {
        __WFI();
    }
}

uint32_t get_tick(void) {
    return g_curr_tick;
}

void timebase_init(void) {
    /* Reload value for 1ms tick */
    SysTick->LOAD = ONE_MSEC_LOAD - 1;

    /* clear systick current value register */
    SysTick->VAL = 0;

    /* select clock source */
    SysTick->CTRL |= STK_CLKSOURCE;

    /*enable interrupt*/
    SysTick->CTRL |= STK_TICKINT;

    /*enable systick*/
    SysTick->CTRL |= STK_ENABLE;

    /*enable global interrupt*/
    __enable_irq();
}

void SysTick_Handler(void) {
    g_curr_tick += TICK_FREQ_MS;
}

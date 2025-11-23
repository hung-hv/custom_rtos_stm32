#ifndef __OS_KERNAL_H__
#define __OS_KERNAL_H__
#include <stdint.h>
#include "stm32f4xx.h"

/* Status Register -> Update Int Flag*/
#define SR_UIF (1U << 0)

void osKernalStackInit(uint8_t i);
void osKernelLaunch(uint32_t quanta);

uint8_t osKernelAddThreads(void (*task0)(void),
                           void (*task1)(void),
                           void (*task2)(void));


void osSchedulerLaunch(void);

void osThreadYeild(void);

void task3(void);

void tim2_1hz_interrupt_init(void);

/* Semaphore */
void osSemaphoreCreate(uint32_t *semaphore, uint32_t initial_count);
void osSemaphoreWait(uint32_t *semaphore);
void osSemaphoreGive(uint32_t *semaphore);
#endif

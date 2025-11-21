#ifndef __OS_KERNAL_H__
#define __OS_KERNAL_H__
#include <stdint.h>
#include "stm32f4xx.h"

void osKernalStackInit(uint8_t i);
void osKernelLaunch(uint32_t quanta);

uint8_t osKernelAddThreads(void (*task0)(void),
                           void (*task1)(void),
                           void (*task2)(void));


void osSchedulerLaunch(void);

#endif

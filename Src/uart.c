#include "uart.h"
#include "stm32f4xx.h"

#define GPIOAEN 	(1U << 0)

void uart_tx_init(void) {
	/* enable UART2 - PA2-Tx, PA3-Rx*/
	/* enable port RCC */
	RCC->AHB1ENR |= GPIOAEN;

	/*set mode PA2 to alternate function mode: 10*/
	GPIOA->MODER |=  (1U << 5);
	GPIOA->MODER &= ~(1U << 4);

	/* set alternate function pin to UART TX*/
	GPIOA->AFR[0]  &= ~(1U << 11);
	GPIOA->AFR[0]  |=  (1U << 10);
	GPIOA->AFR[0]  |=  (1U << 9);
	GPIOA->AFR[0]  |=  (1U << 8);

}

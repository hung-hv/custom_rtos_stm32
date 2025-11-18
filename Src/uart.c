#include <stdio.h>
#include <stdint.h>
#include "uart.h"
#include "stm32f4xx.h"

#define GPIOAEN 	(1U << 0)
#define USART2EN 	(1U << 17)

#define SYS_FREQ	16000000
#define APB1_CLK	SYS_FREQ
#define UART_BAUDRATE	115200
#define UART_CR1_TE		(1U << 3)
#define UART_CR1_UE		(1U << 13)

#define SR_TXE			(1U << 7)

static void uart_write(int data);
static void uart_set_baudrate(uint32_t periph_clk, uint32_t baudrate);

int __io_putchar(int ch){
	uart_write(ch);
	return ch;
}

void uart_tx_init(void) {
	/* enable UART2 - PA2-Tx, PA3-Rx*/
	/* enable port RCC */
	RCC->AHB1ENR |= GPIOAEN;

	/*set mode PA2 to alternate function mode: 10*/
	GPIOA->MODER |=  (1U << 5);
	GPIOA->MODER &= ~(1U << 4);

	/* set alternate function pin to UART TX PF7: 0111*/
	GPIOA->AFR[0]  &= ~(1U << 11);
	GPIOA->AFR[0]  |=  (1U << 10);
	GPIOA->AFR[0]  |=  (1U << 9);
	GPIOA->AFR[0]  |=  (1U << 8);

	/* enable clock for UART2 */
	RCC->APB1ENR != USART2EN;

	/* Config baudrate*/
	uart_set_baudrate(APB1_CLK, UART_BAUDRATE);

	/* Config transfer direction: set TE in CR1 */
	USART2->CR1 |= UART_CR1_TE;
	/* enable UART module by set bit UE in CR1*/
	USART2->CR1 |= UART_CR1_UE;

}

static void uart_write(int data) {
	/* check transmit status */
	while (!(USART2->SR & SR_TXE)) {}

	USART2->DR = (data & 0xFF);
}

static void uart_set_baudrate(uint32_t periph_clk, uint32_t baudrate) {
	USART2-> BRR = (periph_clk + (baudrate / 2U)) / baudrate;
}

#include "led.h"

#define GPIOCEN 	(1U << 2)
#define LED_PIN 	(1U << 13)

void led_init(void) {
	/* Enable access to Port A*/
	RCC->AHB1ENR |= GPIOCEN;

	/* Set pin as output - PA5 */
	GPIOC->MODER |= (1U << 26);
	GPIOC->MODER &= ~(1U << 27);
}

void led_on(void) {
	/* Set ODR on GPIOA */
	GPIOC->ODR |= LED_PIN;
}

void led_off(void) {
	/* CLear ODR bit on GPIOA */
	GPIOC->ODR &= ~LED_PIN;
}

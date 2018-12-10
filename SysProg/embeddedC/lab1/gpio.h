#define RCC_AHB1ENR		(*(unsigned long *)(0x40023830))	/* AHB peripheral clock enable register */
#define GPIOD_MODER		(*(unsigned long *)(0x40020C00))	/* GPIOD mode register */
#define GPIOE_PUPDR		(*(unsigned long *)(0x4002100C))	/* GPIOE pull-up register */

static inline void gpio_init(void) {
	RCC_AHB1ENR		|= 0x00000018;	/* Enable GPIO port D (LEDs) and E (buttons) */
	GPIOD_MODER		|= (0x5555UL << 16);	/* Set GPIOD 8-15 as outputs */
	GPIOE_PUPDR		|= (0xAAAAUL << 16);	/* GPIOE 8-15 inputs by default, but add pull-down resistors */
}

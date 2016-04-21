
#ifndef drv_avr_gpio_h
#define drv_avr_gpio_h

#include <avr/io.h>

#include "drv/gpio/gpio.h"

extern const struct drv_gpio drv_gpio_avr;

struct drv_gpio_avr_data {
	volatile uint8_t *port;
	uint8_t pin;

	volatile uint8_t *_in;
	volatile uint8_t *_ddr;
	uint8_t _mask;
};

#endif

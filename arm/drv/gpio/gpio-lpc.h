
#ifndef drv_gpio_lpc_h
#define drv_gpio_lpc_h

#include "drv/gpio/gpio.h"

extern const struct drv_gpio drv_gpio_lpc;

struct drv_gpio_lpc_data {
	uint8_t port;
};

#endif

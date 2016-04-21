
#ifndef drv_button_gpio_h
#define drv_button_gpio_h

#include "drv/button/button.h"
#include "drv/gpio/gpio.h"

extern const struct drv_button drv_button_gpio;

struct drv_button_gpio_data {
	struct dev_gpio *dev_gpio;
	gpio_pin pin;
};

#endif

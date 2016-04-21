
#ifndef drv_led_gpio_h
#define drv_led_gpio_h

#include "drv/led/led.h"
#include "bios/gpio.h"

extern const struct drv_led drv_led_gpio;

struct drv_led_gpio_data {
	struct dev_gpio *dev_gpio;
	gpio_pin pin;
};

#endif

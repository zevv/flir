#ifndef drv_gpio_h
#define drv_gpio_h

#include "config.h"
#include "bios/gpio.h"

#ifdef DRV_GPIO
#define DEV_INIT_GPIO(name, drv, ...) DEV_INIT(GPIO, gpio, name, drv,__VA_ARGS__) 
#else
#define DEV_INIT_GPIO(name, drv, ...)
#endif

struct dev_gpio {
	const struct drv_gpio *drv;
	void *drv_data;
	rv init_status;
};

struct drv_gpio {
	rv (*init)(struct dev_gpio *gpio);
	uint8_t (*get_pin_count)(struct dev_gpio *gpio);
	rv (*set_pin_direction)(struct dev_gpio *gpio, gpio_pin pin, gpio_direction direction);
	rv (*set_pin)(struct dev_gpio *gpio, gpio_pin pin, uint8_t onoff);
	rv (*get_pin)(struct dev_gpio *gpio, gpio_pin pin, uint8_t *onoff);
	rv (*set)(struct dev_gpio *gpio, uint32_t val, uint32_t mask);
};

#endif

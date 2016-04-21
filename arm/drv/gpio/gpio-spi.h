#ifndef drv_gpio_spi_h
#define drv_gpio_spi_h

#include "drv/gpio/gpio.h"
#include "bios/spi.h"

struct drv_gpio_spi_data {
	struct dev_spi *spi_dev;
	struct dev_gpio *clock_gpio;
	gpio_pin clock_pin;
	uint8_t shadow;
};

extern const struct drv_gpio drv_gpio_spi;

#endif



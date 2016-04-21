
#ifndef drv_gpio_stm32_h
#define drv_gpio_stm32_h

#include "drv/gpio/gpio.h"
#include "stm32f1xx_hal.h"

extern const struct drv_gpio drv_gpio_stm32;

struct drv_gpio_stm32_data {
	GPIO_TypeDef *port;
};

#endif


#ifndef drv_stm32_button_h
#define drv_stm32_button_h

#include "drv/button/button.h"
#include "stm32f1xx_hal.h"

struct drv_button_stm32_data {
	GPIO_TypeDef *port;
	uint8_t pin;
};

extern struct drv_button drv_button_stm32;

#endif

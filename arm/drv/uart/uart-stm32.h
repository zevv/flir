#ifndef drv_stm32_uart_h
#define drv_stm32_uart_h

#include "drv/uart/uart.h"
#include "stm32f1xx_hal.h"

struct drv_uart_stm32_data {
	int8_t uart;
	UART_HandleTypeDef _huart;
	int _ready;
};

extern struct drv_uart drv_uart_stm32;

#endif

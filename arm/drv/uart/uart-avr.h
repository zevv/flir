#ifndef drv_avr_uart_h
#define drv_avr_uart_h

#include "drv/uart/uart.h"

struct drv_uart_avr_data {
	int8_t uart;
};

extern struct drv_uart drv_uart_avr;

#endif

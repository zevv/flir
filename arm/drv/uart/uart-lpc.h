#ifndef drv_lpc_uart_h
#define drv_lpc_uart_h

#include "drv/uart/uart.h"

struct drv_uart_lpc_data {
	int _ready;
};

extern struct drv_uart drv_uart_lpc;

#endif

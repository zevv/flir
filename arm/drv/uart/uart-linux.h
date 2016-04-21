#ifndef drv_linux_uart_h
#define drv_linux_uart_h

#include "drv/uart/uart.h"

struct drv_uart_linux_data {
	const char *dev;
	uint32_t baudrate;

	int _fd;
};

extern const struct drv_uart drv_uart_linux;

#endif

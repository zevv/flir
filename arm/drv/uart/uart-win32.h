#ifndef drv_win32_uart_h
#define drv_win32_uart_h

#include "drv/uart/uart.h"

struct drv_uart_win32_data {
	int fd_read;
	int fd_write;
};

extern const struct drv_uart drv_uart_win32;

#endif

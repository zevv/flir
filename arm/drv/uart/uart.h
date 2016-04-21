#ifndef drv_uart_h
#define drv_uart_h

#include "config.h"
#include "bios/uart.h"

#ifdef DRV_UART
#define DEV_INIT_UART(name, drv, ...) DEV_INIT(UART, uart, name, drv, __VA_ARGS__) 
#else
#define DEV_INIT_UART(name, drv, ...)
#endif

struct dev_uart {
	const struct drv_uart *drv;
	void *drv_data;
	rv init_status;
};

struct drv_uart {
	rv (*init)(struct dev_uart *uart);
	rv (*set_speed)(struct dev_uart *uart, uint16_t baudrate);
	rv (*tx)(struct dev_uart *uart, uint8_t c);
};


#endif

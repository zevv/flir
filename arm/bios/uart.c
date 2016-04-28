
#include "bios/dev.h"
#include "bios/bios.h"
#include "bios/uart.h"
#include "drv/uart/uart.h"

rv uart_init(struct dev_uart *dev)
{
	rv r = dev->drv->init(dev);
	if(r == RV_OK) {
		r = uart_set_speed(dev, 9600);
	}
	return r;
}


rv uart_tx2(struct dev_uart *uart, uint8_t c)
{
	return uart->drv->tx(uart, c);
}


rv uart_set_speed(struct dev_uart *uart, uint16_t baudrate)
{
	rv r = RV_OK;;

	if(uart->drv->set_speed != NULL) {
		r = uart->drv->set_speed(uart, baudrate);
	}

	return r;
}

void flir_tx(uint8_t c);

rv uart_tx(uint8_t c)
{
	flir_tx(c);
#ifdef DEFAULT_UART_DEV
	extern struct dev_uart uart0;
	return uart0.drv->tx(&DEFAULT_UART_DEV, (uint8_t)c);
#else
	return RV_OK;
#endif
}


/*
 * End
 */

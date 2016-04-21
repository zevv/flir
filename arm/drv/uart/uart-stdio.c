
#include <stdio.h>

#include "bios/bios.h"
#include "bios/evq.h"
#include "bios/uart.h"
#include "drv/uart/uart.h"
#include "drv/uart/uart-stdio.h"

static rv init(struct dev_uart *uart)
{
	return RV_OK;
}


static rv tx(struct dev_uart *uart, uint8_t c)
{
	putchar(c);
	return RV_OK;
}


const struct drv_uart drv_uart_stdio = {
	.init = init,
	.tx = tx,
};

/*
 * End
 */


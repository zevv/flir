
#include <unistd.h>
#include <stdint.h>

#include "bios/bios.h"
#include "bios/uart.h"
#include "drv/uart/uart-lpc.h"
#include "bios/evq.h"
#include "bios/printd.h"

#include "arch/lpc/inc/chip.h"
#include "arch/lpc/inc/uart_13xx.h"


static rv init(struct dev_uart *dev)
{
	Chip_UART_Init(LPC_USART);
	Chip_UART_SetBaud(LPC_USART, 57600);
	Chip_UART_ConfigData(LPC_USART, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(LPC_USART, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2));
	Chip_UART_TXEnable(LPC_USART);


	Chip_UART_IntEnable(LPC_USART, (UART_IER_RBRINT | UART_IER_RLSINT));

	NVIC_SetPriority(UART0_IRQn, 1);
	NVIC_EnableIRQ(UART0_IRQn);

	return RV_OK;
}


static rv tx(struct dev_uart *dev, uint8_t c)
{
	Chip_UART_SendBlocking(LPC_USART, &c, sizeof(c));

	return RV_OK;
}


void dump_stack(void);

void uart_irq(void)
{

	for(;;) {
		char c;
		int n = Chip_UART_Read(LPC_USART, &c, sizeof(c));
		if(n) {
			if(c == '~') {
				dump_stack();
			} else {
				event_t ev;
				ev.type = EV_UART;
				ev.uart.data = c;
				evq_push(&ev);
			}
		} else {
			break;
		}
	}
}

struct drv_uart drv_uart_lpc = {
	.init = init,
	.tx = tx,
};


/*
 * End
 */

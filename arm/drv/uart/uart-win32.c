
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "bios/bios.h"
#include "bios/evq.h"
#include "bios/uart.h"
#include "drv/uart/uart.h"
#include "drv/uart/uart-win32.h"
#include "arch/win32/mainloop.h"


static int on_read_stdin(HANDLE h, void *data)
{
	DWORD n;
	INPUT_RECORD buffer;
	ReadConsoleInput(h, &buffer, 1, &n);

	if(buffer.Event.KeyEvent.bKeyDown) {
		int c = buffer.Event.KeyEvent.uChar.AsciiChar;

		event_t ev;
		ev.type = EV_UART;
		ev.uart.data = c;
		evq_push(&ev);
	}

	return 0;
}


static rv init(struct dev_uart *uart)
{
	mainloop_handle_add(GetStdHandle(STD_INPUT_HANDLE), MAINLOOP_FD_READ, on_read_stdin, NULL);

	return RV_OK;
}


static rv tx(struct dev_uart *uart, uint8_t c)
{
	putchar(c);
	return RV_OK;
}


const struct drv_uart drv_uart_win32 = {
	.init = init,
	.tx = tx,
};

/*
 * End
 */


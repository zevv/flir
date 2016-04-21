
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <termios.h>

#include "bios/bios.h"
#include "bios/evq.h"
#include "bios/uart.h"
#include "drv/uart/uart.h"
#include "drv/uart/uart-linux.h"
#include "arch/linux/mainloop.h"

struct termios save;

static void at_exit(void)
{
	tcsetattr(0, TCSANOW, &save);
}


static int on_read_stdin(int fd, void *data)
{
	char c;
	read(0, &c, sizeof(c));

	if(c == 3) exit(0);

	event_t ev;
	ev.type = EV_UART;
	ev.uart.data = c;
	evq_push(&ev);

	return 0;
}


static rv init(struct dev_uart *uart)
{
	struct termios tios;
	struct drv_uart_linux_data *dd = uart->drv_data;

	dd->_fd = open(dd->dev, O_RDWR);

	if(dd->_fd != -1) {

		tcgetattr(0, &save);
		memset(&tios, 0, sizeof(tios));

		int br = 0;

		switch(dd->baudrate) {
			case   300:	br =  B300;   break;
			case  1200:	br =  B1200;  break;
			case  2400: 	br =  B2400;  break;
			case  4800:	br =  B4800;  break;
			case  9600:	br =  B9600;  break;
			case 19200:	br =  B19200; break;
			case 38400:	br =  B38400; break;
			case 57600:	br =  B57600; break;
			default:	br = B115200; break;
		}
		
		tios.c_cflag = br | CLOCAL | CREAD | CS8; 
		tios.c_oflag = OPOST | ONLCR;
		tios.c_iflag = IGNPAR;
		tios.c_lflag = 0;
		tios.c_cc[VTIME] = 0;
		tios.c_cc[VMIN]  = 1;

		tcsetattr(dd->_fd, TCSANOW, &tios);
		tcflush(dd->_fd, TCIFLUSH);

		mainloop_fd_add(dd->_fd, FD_READ, on_read_stdin, NULL);

		atexit(at_exit);
	}

	return RV_OK;
}


static rv tx(struct dev_uart *uart, uint8_t c)
{
	struct drv_uart_linux_data *dd = uart->drv_data;
	int r = write(dd->_fd, &c, 1);
	return (r == 0) ? RV_OK : RV_EIO;
}


const struct drv_uart drv_uart_linux = {
	.init = init,
	.tx = tx,
};

/*
 * End
 */


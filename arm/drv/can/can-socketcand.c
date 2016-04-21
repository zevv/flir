
/*
 * Driver for TCP socketcand protocol
 *
 * https://github.com/dschanoeh/socketcand/blob/master/doc/protocol.md
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>

#ifdef _WIN32

#include <winsock2.h>
#include <windows.h>
#include "arch/win32/mainloop.h"

typedef SOCKET socket_t;
typedef int socklen_t;

#else

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include "arch/linux/mainloop.h"

#define SOCKET_ERROR -1
typedef int socket_t;
typedef void *HANDLE;
#define INVALID_SOCKET -1

#endif

#include "bios/bios.h"
#include "bios/can.h"
#include "bios/evq.h"
#include "bios/printd.h"
#include "drv/can/can.h"
#include "drv/can/can-socketcand.h"


static int on_read_can(int fd, void *ptr);


#ifdef _WIN32

static int on_read_can_handle(HANDLE sockh, void *user)
{
	struct dev_can *dev = user;
	struct drv_can_socketcand_data *dd = dev->drv_data;
	WSAResetEvent(dd->sockh);
	return on_read_can(0, dev);
}

#endif


static void send_cmd(struct drv_can_socketcand_data *dd, const char *fmt, ...)
{
	va_list va;
	char buf[256];

	va_start(va, fmt);
	vsnprintf(buf, sizeof(buf), fmt, va);
	va_end(va);

	int r = send(dd->sock, buf, strlen(buf), 0);

	if(r == SOCKET_ERROR) {
		fprintf(stderr, "Error sending open command\n");
	}
}


static rv init(struct dev_can *dev)
{
	struct drv_can_socketcand_data *dd = dev->drv_data;
	int r;

	dd->sock = socket(AF_INET, SOCK_STREAM, 0);
	if(dd->sock == INVALID_SOCKET) {
		fprintf(stderr, "Error opening CAN socket: %s\n", strerror(errno));
		return RV_EIO;
	}

	struct sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_port = htons(29536);
	sa.sin_addr.s_addr = inet_addr(dd->host);

	r = connect(dd->sock, (struct sockaddr *)&sa, sizeof(sa));
	if(r == SOCKET_ERROR) {
		fprintf(stderr, "Error connecting to socketcand\n");
		return RV_EIO;
	}

	send_cmd(dd, "< open %s >", dd->ifname);
	send_cmd(dd, "< rawmode >");

#ifdef _WIN32
	dd->sockh = WSACreateEvent();
	WSAEventSelect(dd->sock, dd->sockh, FD_READ);
	mainloop_handle_add(dd->sockh, FD_READ, on_read_can_handle, dev);
#else
	mainloop_fd_add(dd->sock, FD_READ, on_read_can, dev);
#endif

	return RV_OK;
}


static void handle_frame(struct dev_can *dev, const char *buf)
{
	unsigned int id;
	float ts;
	unsigned int data[8];

	int n = sscanf(buf + 8, "%x %f %2x%2x%2x%2x%2x%2x%2x%2x",
			&id, &ts,
			&data[0], &data[1], &data[2], &data[3],
			&data[4], &data[5], &data[6], &data[7]);

	if(n >= 2) {
		event_t e;
		e.type = EV_CAN;
		e.can.dev = dev;
		e.can.extended = 0;
		e.can.id = id;
		e.can.len = n - 2;
		int i;
		for(i=0; i<n-2; i++) {
			e.can.data[i] = data[i];
		}
		evq_push(&e);
	}
}


static int on_read_can(int fd, void *ptr)
{
	struct dev_can *dev = ptr;
	struct drv_can_socketcand_data *dd = dev->drv_data;

	char buf[256];
	int r;

	r = recv(dd->sock, buf, sizeof(buf)-1, 0);

	if(r > 0) {

		buf[r] = '\0';
			
		// < frame 58A 1450608841.139876 43001000A2010000 >

		if(!strncmp("< frame ", buf, 8)) {
			handle_frame(dev, buf);
		}
	}

	return 0;
}


static rv tx(struct dev_can *dev, enum can_address_mode_t fmt, can_addr_t addr, const void *data, size_t len)
{
	struct drv_can_socketcand_data *dd = dev->drv_data;
	char buf[256];
	int l = 0;
	const uint8_t *p = data;

	l += snprintf(buf + l, sizeof(buf) - l, "< send %x %d", addr, len);

	size_t i;
	for(i=0; i<len; i++) {
		l += snprintf(buf + l, sizeof(buf) - l, " %02x", p[i]);
	}

	l += snprintf(buf + l, sizeof(buf) - l, " >");

	send_cmd(dd, buf);

	return RV_OK;
}


const struct drv_can drv_can_socketcand = {
	.init = init,
	.tx = tx
};


/*
 * End
 */

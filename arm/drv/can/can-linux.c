#ifndef _lint

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/poll.h>
#include <sys/time.h>

#include "bios/bios.h"
#include "bios/can.h"
#include "bios/evq.h"
#include "bios/printd.h"
#include "bios/log.h"
#include "drv/can/can.h"
#include "drv/can/can-linux.h"
#include "arch/linux/mainloop.h"
#include "arch/linux/arch.h"


static int on_read_can(int fd, void *ptr);

static rv init(struct dev_can *dev)
{
	struct drv_can_linux_data *dd = dev->drv_data;

	int r;
	struct ifreq ifr;
	struct sockaddr_can addr;

	dd->fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if(dd->fd == -1) {
		logd(LG_WRN, "Error opening CAN socket: %s", strerror(errno));
		return RV_EIO;
	}

	strcpy(ifr.ifr_name, dd->ifname);
	r = ioctl(dd->fd, SIOCGIFINDEX, &ifr);
	if(r == -1) {
		logd(LG_WRN, "Error setting CAN interface '%s': %s", dd->ifname, strerror(errno));
		return RV_EIO;
	}

	memset(&addr, 0, sizeof(addr));
	addr.can_ifindex = ifr.ifr_ifindex;
	addr.can_family = AF_CAN;

	r = bind(dd->fd, (struct sockaddr *)&addr, sizeof(addr));
	if(r == -1) {
		logd(LG_WRN, "Error setting CAN interface: %s", strerror(errno));
		return RV_EIO;
	}

	mainloop_fd_add(dd->fd, FD_READ, on_read_can, dev);

	return RV_OK;
}


static int on_read_can(int fd, void *ptr)
{
	struct dev_can *dev = ptr;
	struct drv_can_linux_data *dd = dev->drv_data;

	struct can_frame frame;
	int r;

	r = recv(dd->fd, &frame, sizeof frame, 0);

	if(r > 0) {
		event_t e;
		e.type = EV_CAN;
		e.can.dev = dev;
		e.can.extended = !!(frame.can_id & CAN_EFF_FLAG);
		e.can.id = frame.can_id & CAN_EFF_MASK;
		e.can.len = frame.can_dlc;
		memcpy(e.can.data, frame.data, frame.can_dlc);
		evq_push(&e);
		dd->_rx_total ++;
	} else {
		logd(LG_WRN, "CAN rx: %s", strerror(errno));
	}

	return 0;
}


static rv tx(struct dev_can *dev, enum can_address_mode_t fmt, can_addr_t addr, const void *data, size_t len)
{
	struct drv_can_linux_data *dd = dev->drv_data;

	assert(dd->fd > 0);

	struct can_frame frame;
	memset(&frame, 0, sizeof frame);

	frame.can_id = addr;
	if(fmt == CAN_ADDRESS_MODE_EXT) frame.can_id |= CAN_EFF_FLAG;

	frame.can_dlc = len;
	memcpy(&frame.data, data, len);

	int e = send(dd->fd, &frame, sizeof frame, 0);

	rv r = RV_OK;

	if(e > 0) {
		dd->_tx_total ++;
	} else {
		dd->_tx_err ++;
		r = errno_to_rv(errno);
	}
	
	return r;
}


static rv get_stats(struct dev_can *dev, struct can_stats *stats)
{
	struct drv_can_linux_data *dd = dev->drv_data;

	stats->tx_total = dd->_tx_total;
	stats->tx_err = dd->_tx_err;
	
	stats->rx_total = dd->_rx_total;

	return RV_OK;
}


const struct drv_can drv_can_linux = {
	.init = init,
	.tx = tx,
	.get_stats = get_stats,
};


#endif

/*
 * End
 */

#ifndef drv_linux_can_h
#define drv_linux_can_h

#include "drv/can/can.h"

struct drv_can_linux_data {
	uint8_t channel;
	const char *ifname;
	int fd;

	uint16_t _tx_total;
	uint16_t _tx_err;
	uint16_t _rx_total;
};

extern const struct drv_can drv_can_linux;

#endif


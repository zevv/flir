
#ifndef drv_relay_gpio_h
#define drv_relay_gpio_h

#include "drv/relay/relay.h"

extern const struct drv_relay drv_relay_gpio;

struct drv_relay_gpio_data {
	struct dev_gpio *dev_gpio;
	gpio_pin pin;
};

#endif

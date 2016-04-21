

#include <stdio.h>
#include <assert.h>

#include "bios/bios.h"
#include "bios/printd.h"
#include "bios/relay.h"
#include "bios/gpio.h"
#include "drv/relay/relay.h"
#include "drv/relay/relay-gpio.h"

static rv init(struct dev_relay *dev)
{
	struct drv_relay_gpio_data *dd = dev->drv_data;
	assert(dd->dev_gpio);
	(void)gpio_set_pin_direction(dd->dev_gpio, dd->pin, GPIO_DIR_OUTPUT);

	return RV_OK;
}


static rv set(struct dev_relay *dev, enum relay_state_t state)
{
	rv r = RV_OK;

	struct drv_relay_gpio_data *dd = dev->drv_data;
	assert(dd->dev_gpio);

	if(state == RELAY_STATE_OPEN) {
		r = gpio_set_pin(dd->dev_gpio, dd->pin, 0);
	}

	if(state == RELAY_STATE_CLOSED) {
		r = gpio_set_pin(dd->dev_gpio, dd->pin, 1);
	}

	return r;
}


const struct drv_relay drv_relay_gpio = {
	.init = init,
	.set = set,
};

/*
 * End
 */


#include <stdio.h>
#include <assert.h>

#include "bios/bios.h"
#include "bios/gpio.h"
#include "drv/led/led.h"
#include "drv/led/led-gpio.h"

static rv init(struct dev_led *dev)
{
	const struct drv_led_gpio_data *dd = dev->drv_data;
	assert(dd->dev_gpio);
	(void)gpio_set_pin_direction(dd->dev_gpio, dd->pin, GPIO_DIR_OUTPUT);

	return RV_OK;
}


static rv set(struct dev_led *dev, uint8_t onoff)
{
	const struct drv_led_gpio_data *dd = dev->drv_data;
	return gpio_set_pin(dd->dev_gpio, dd->pin, onoff);
}


const struct drv_led drv_led_gpio = {
	.init = init,
	.set = set,
};

/*
 * End
 */

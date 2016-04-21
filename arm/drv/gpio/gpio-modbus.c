
#include <stdint.h>

#include "bios/bios.h"
#include "bios/gpio.h"
#include "drv/gpio/gpio-dummy.h"
#include "bios/printd.h"
#include "plat.h"


static rv init(struct dev_gpio *gpio)
{
	return RV_OK;
}


static rv set_pin_direction(struct dev_gpio *gpio, gpio_pin pin, gpio_direction direction)
{
	return RV_OK;
}


static rv set_pin(struct dev_gpio *gpio, gpio_pin pin, uint8_t onoff)
{
	return RV_OK;
}


static rv get_pin(struct dev_gpio *gpio, gpio_pin pin, uint8_t *onoff)
{
	return RV_OK;
}


const struct drv_gpio drv_gpio_dummy = {
	.init = init,
	.set_pin_direction = set_pin_direction,
	.set_pin = set_pin,
	.get_pin = get_pin,
};

/*
 * End
 */

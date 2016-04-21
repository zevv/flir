
#include <assert.h>

#include "bios/bios.h"
#include "bios/gpio.h"
#include "bios/spi.h"
#include "drv/gpio/gpio-spi.h"


static rv update(struct drv_gpio_spi_data *dd)
{
	rv r;
	r = spi_write(dd->spi_dev, &dd->shadow, sizeof dd->shadow);
	if(r == RV_OK) {
		r = gpio_set_pin(dd->clock_gpio, dd->clock_pin, 1);
	}
	if(r == RV_OK) {
		r = gpio_set_pin(dd->clock_gpio, dd->clock_pin, 0);
	}
	return r;
}


static rv init(struct dev_gpio *gpio)
{
	struct drv_gpio_spi_data *dd = gpio->drv_data;

	dd->shadow = 0;

	(void)gpio_set_pin_direction(dd->clock_gpio, dd->clock_pin, GPIO_DIR_OUTPUT);
	(void)gpio_set_pin(dd->clock_gpio, dd->clock_pin, 0);

	return RV_OK;
}


static rv set_pin_direction(struct dev_gpio *gpio, gpio_pin pin, gpio_direction direction)
{
	return RV_EIMPL;
}


static rv set_pin(struct dev_gpio *gpio, gpio_pin pin, uint8_t onoff)
{
	struct drv_gpio_spi_data *dd = gpio->drv_data;

	if(onoff != 0u) {
		dd->shadow |= (uint8_t)(1u<<pin);
	} else {
		dd->shadow &= (uint8_t)~(1u<<pin);
	}

	return update(dd);
}


static rv get_pin(struct dev_gpio *gpio, gpio_pin pin, uint8_t *onoff)
{
	return RV_EIMPL;
}


static rv set(struct dev_gpio *gpio, uint32_t val, uint32_t mask)
{
	struct drv_gpio_spi_data *dd = gpio->drv_data;

	dd->shadow &= (uint8_t)~mask;
	dd->shadow |= (uint8_t)val;
	return update(dd);
}


const struct drv_gpio drv_gpio_spi = {
	.init = init,
	.set_pin_direction = set_pin_direction,
	.set_pin = set_pin,
	.get_pin = get_pin,
	.set = set,
};

/*
 * End
 */

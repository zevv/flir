
#include <avr/io.h>

#include "bios/bios.h"
#include "bios/gpio.h"
#include "drv/gpio/gpio-avr.h"
#include "bios/printd.h"
#include "plat.h"

static rv init(struct dev_gpio *dev)
{
	struct drv_gpio_avr_data *dd = dev->drv_data;

	if(dd->port == &PORTA) {
		dd->_in = &PINA;
		dd->_ddr = &DDRA;
	}
	if(dd->port == &PORTB) {
		dd->_in = &PINB;
		dd->_ddr = &DDRB;
	}
	if(dd->port == &PORTC) {
		dd->_in = &PINC;
		dd->_ddr = &DDRC;
	}
	if(dd->port == &PORTD) {
		dd->_in = &PIND;
		dd->_ddr = &DDRD;
	}

	return RV_OK;
}


static rv set_pin_direction(struct dev_gpio *dev, gpio_pin pin, gpio_direction direction)
{
	struct drv_gpio_avr_data *dd = dev->drv_data;

	if(direction == GPIO_DIR_INPUT) {
		*(dd->_ddr) &= ~(1<<pin);
	} else {
		*(dd->_ddr) |=  (1<<pin);
	}

	return RV_OK;
}


static rv set_pin(struct dev_gpio *dev, gpio_pin pin, uint8_t onoff)
{
	struct drv_gpio_avr_data *dd = dev->drv_data;

	if(onoff) {
		*(dd->port) |=  (1<<pin);
	} else {
		*(dd->port) &= ~(1<<pin);
	}

	return RV_OK;
}


static rv get_pin(struct dev_gpio *dev, gpio_pin pin, uint8_t *onoff)
{
	struct drv_gpio_avr_data *dd = dev->drv_data;

	*onoff = !!(*dd->_in & (1<<pin));

	return RV_OK;
}


const struct drv_gpio drv_gpio_avr = {
	.init = init,
	.set_pin_direction = set_pin_direction,
	.set_pin = set_pin,
	.get_pin = get_pin,
};

/*
 * End
 */

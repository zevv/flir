
#include <stdint.h>

#include "bios/bios.h"
#include "bios/dev.h"
#include "bios/printd.h"
#include "bios/gpio.h"
#include "bios/cmd.h"

#include "lib/atoi.h"

#include "drv/gpio/gpio.h"


rv gpio_init(struct dev_gpio *dev)
{
	return dev->drv->init(dev);
}


uint8_t gpio_get_pin_count(struct dev_gpio *dev)
{
	uint8_t count = 0;

	if(dev->drv->get_pin_count != NULL) {
		count = dev->drv->get_pin_count(dev);
	}

	return count;
}


rv gpio_set_pin_direction(struct dev_gpio *dev, gpio_pin pin, gpio_direction dir)
{
	rv r = dev->init_status;
	if(r == RV_OK) {
		r = dev->drv->set_pin_direction(dev, pin, dir);
	}
	return r;
}


rv gpio_set_pin(struct dev_gpio *dev, gpio_pin pin, uint8_t onoff)
{
	rv r = dev->init_status;
	if(r == RV_OK) {
		r = dev->drv->set_pin(dev, pin, onoff);
	}
	return r;
}


rv gpio_get_pin(struct dev_gpio *dev, gpio_pin pin, uint8_t *onoff)
{
	rv r = dev->init_status;
	if(r == RV_OK) {
		r = dev->drv->get_pin(dev, pin, onoff);
	}
	return r;
}


rv gpio_set(struct dev_gpio *dev, uint32_t val, uint32_t mask)
{
	rv r = dev->init_status;
	if(r == RV_OK) {
		r = dev->drv->set(dev, val, mask);
	}
	return r;
}


static void print_gpio(struct dev_descriptor *dd, void *ptr)
{
	struct dev_gpio *dev = dd->dev;
	uint8_t count = gpio_get_pin_count(dev);
	uint8_t i;
	printd("%s:", dd->name);
	for(i=0; i<count; i++) {
		uint8_t v;
		(void)gpio_get_pin(dev, i, &v);
		if((i & 3u) == 0u) {
			printd(" ");
		}
		printd("%d", v);
	}
	printd("\n");
}

static rv on_cmd_gpio(uint8_t argc, char **argv)
{
	rv r = RV_EINVAL;

	if(argc >= 1u) {
		char cmd = argv[0][0];

		if(cmd == 'l') {
			dev_foreach(DEV_TYPE_GPIO, print_gpio, NULL);
			r = RV_OK;
		}

		if((cmd == 's') && (argc >= 3u)) {
			const struct dev_descriptor *dd = dev_find(argv[1], DEV_TYPE_GPIO);
			if(dd != NULL) {
				uint8_t pin = (uint8_t)a_to_s32(argv[2]);
				uint8_t v = (uint8_t)a_to_s32(argv[3]);
				r = gpio_set_pin(dd->dev, pin, v);
			} else {
				r = RV_ENODEV;
			}
		}
	}
	return r;
}

CMD_REGISTER(gpio, on_cmd_gpio, "l[ist] | s[et] <id> <pin> <0|1>");


/*
 * End
 */



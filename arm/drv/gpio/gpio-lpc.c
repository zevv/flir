
#include "arch/lpc/inc/chip.h"
#include "arch/lpc/inc/gpio_13xx_2.h"

#include <stdint.h>

#include "bios/bios.h"
#include "bios/gpio.h"
#include "drv/gpio/gpio-lpc.h"
#include "bios/printd.h"
#include "plat.h"

#include "arch/lpc/inc/chip.h"
#include "arch/lpc/inc/uart_13xx.h"

static rv init(struct dev_gpio *gpio)
{
	Chip_GPIO_Init(LPC_GPIO_PORT);

	return RV_OK;
}


static rv set_pin_direction(struct dev_gpio *dev, gpio_pin pin, gpio_direction direction)
{
	struct drv_gpio_lpc_data *dd = dev->drv_data;
	Chip_GPIO_WriteDirBit(LPC_GPIO_PORT, dd->port, pin, direction == GPIO_DIR_OUTPUT);
	return RV_OK;
}


static rv set_pin(struct dev_gpio *dev, gpio_pin pin, uint8_t onoff)
{
	struct drv_gpio_lpc_data *dd = dev->drv_data;
	Chip_GPIO_WritePortBit(LPC_GPIO_PORT, dd->port, pin, !onoff);
	return RV_OK;
}


static rv get_pin(struct dev_gpio *dev, gpio_pin pin, uint8_t *onoff)
{
	struct drv_gpio_lpc_data *dd = dev->drv_data;
	*onoff = Chip_GPIO_ReadPortBit(LPC_GPIO_PORT, dd->port, pin);
	return RV_OK;
}


static uint8_t get_pin_count(struct dev_gpio *dev)
{
	return 8u;
}


const struct drv_gpio drv_gpio_lpc = {
	.init = init,
	.set_pin_direction = set_pin_direction,
	.get_pin_count = get_pin_count,
	.set_pin = set_pin,
	.get_pin = get_pin,
};

/*
 * End
 */


#include "arch/stm32/sb_stm32f10x.h"
#include "drv/arm/fan.h"
#include "arch/stm32/gpio.h"
#include "bios/fan.h"
#include "drv/fan.h"

static rv init(struct dev_fan *dev)
{
	struct drv_fan_stm32_data *dd = dev->drv_data;

	if(dd->port == GPIO_PORT_A) RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
	if(dd->port == GPIO_PORT_B) RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
	if(dd->port == GPIO_PORT_C) RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

	stm32_gpio_set_mode(dd->port, dd->pin, GPIO_MODE_OUT);

	return RV_OK;
}


static void set_speed(struct dev_fan *dev, uint8_t speed)
{
	uint8_t onoff = (speed > 50) ? 1 : 0;

	struct drv_fan_stm32_data *dd = dev->drv_data;
	stm32_gpio_set(dd->port, dd->pin, onoff);
}


struct drv_fan drv_fan_stm32 = {
	.init = init,
	.set_speed = set_speed,
};

/*
 * End
 */

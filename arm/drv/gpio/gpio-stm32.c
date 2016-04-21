
#include "plat.h"

#include "bios/bios.h"
#include "bios/gpio.h"
#include "bios/printd.h"

#include "drv/gpio/gpio-stm32.h"

#include "stm32f1xx_hal.h"


static rv init(struct dev_gpio *dev)
{
	struct drv_gpio_stm32_data *dd = dev->drv_data;

	if(dd->port == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
	if(dd->port == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
	if(dd->port == GPIOC) __HAL_RCC_GPIOC_CLK_ENABLE();

	return RV_OK;
}


static uint8_t get_pin_count(struct dev_gpio *dev)
{
	return 16u;
}


static rv set_pin_direction(struct dev_gpio *dev, gpio_pin pin, gpio_direction direction)
{
	struct drv_gpio_stm32_data *dd = dev->drv_data;

	GPIO_InitTypeDef  GPIO_InitStruct;

	GPIO_InitStruct.Pin = (1 << pin);
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;

	if(direction == GPIO_DIR_INPUT) {
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	}

	if(direction == GPIO_DIR_OUTPUT) {
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	}
	
	HAL_GPIO_Init(dd->port, &GPIO_InitStruct);

	return RV_OK;
}


static rv set_pin(struct dev_gpio *dev, gpio_pin pin, uint8_t onoff)
{
	struct drv_gpio_stm32_data *dd = dev->drv_data;

	HAL_GPIO_WritePin(dd->port, (1<<pin), onoff == 0 ? GPIO_PIN_RESET : GPIO_PIN_SET);

	return RV_OK;
}


static rv get_pin(struct dev_gpio *dev, gpio_pin pin, uint8_t *onoff)
{
	struct drv_gpio_stm32_data *dd = dev->drv_data;
	*onoff = HAL_GPIO_ReadPin(dd->port, (1<<pin));
	return RV_OK;
}


const struct drv_gpio drv_gpio_stm32 = {
	.init = init,
	.get_pin_count = get_pin_count,
	.set_pin_direction = set_pin_direction,
	.set_pin = set_pin,
	.get_pin = get_pin,
};

/*
 * End
 */

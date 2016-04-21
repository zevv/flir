
#include <stdint.h>

#include "bios/bios.h"
#include "drv/button/button-gpio.h"
#include "bios/button.h"
#include "bios/gpio.h"
#include "drv/button/button.h"

#include "bios/evq.h"
#include "bios/printd.h"



static rv init(struct dev_button *dev)
{
	struct drv_button_gpio_data *dd = dev->drv_data;
	(void)gpio_set_pin_direction(dd->dev_gpio, dd->pin, GPIO_DIR_INPUT);

	return RV_OK;
}


static enum button_state get(struct dev_button *dev)
{
	struct drv_button_gpio_data *dd = dev->drv_data;

	uint8_t v;
	(void)gpio_get_pin(dd->dev_gpio, dd->pin, &v);
	return (v == 0u) ? BUTTON_STATE_PUSH : BUTTON_STATE_RELEASE;

} 


const struct drv_button drv_button_gpio = {
	.init = init,
	.get = get,
};


/*
 * End
 */


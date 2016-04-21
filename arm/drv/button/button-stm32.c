
#include <stdint.h>

#include "drv/button/button-stm32.h"
#include "bios/button.h"
#include "drv/button/button.h"

#include "bios/evq.h"
#include "bios/printd.h"

#define TRUE 1

#define DEBOUNCING_FILTER 10


static rv init(struct dev_button *dev)
{
	//struct drv_button_stm32_data *dd = dev->drv_data;
	//stm32_gpio_set_mode(dd->port, dd->pin, GPIO_MODE_IN);


	return RV_OK;
}


//enum button_state get (struct dev_button *dev)
//{
//	//struct drv_button_stm32_data *dd = dev->drv_data;
//	uint8_t value;
//	value = 0; //stm32_gpio_get(dd->port, dd->pin);
//	enum button_state button_state_return;
//	if (value == 1){
//		button_state_return = BUTTON_STATE_PUSH;
//	}
//	else{
//		button_state_return = BUTTON_STATE_RELEASE; 
//	}
//	return button_state_return;
//} 


struct drv_button drv_button_stm32 = {
	.init = init,
//	.get = get,
};


/*
 * End
 */


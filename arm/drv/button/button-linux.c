
#include <stdint.h>

#include "bios/bios.h"
#include "bios/button.h"
#include "bios/printd.h"
#include "drv/button/button.h"


static rv init(struct dev_button *button)
{
	return RV_OK;
}


static enum button_state get(struct dev_button *dev)
{
	return BUTTON_STATE_RELEASE;
} 


const struct drv_button drv_button_linux = {
	.init = init,
	.get = get,
};

/*
 * End
 */

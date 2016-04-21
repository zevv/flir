
#include "bios/fan.h"
#include "drv/fan/fan-linux.h"
#include "bios/printd.h"


static rv init(struct dev_fan *fan)
{
	return RV_OK;
}



static void set_speed(struct dev_fan *fan, uint8_t speed)
{
}


const struct drv_fan drv_fan_linux = {
	.init = init,
	.set_speed = set_speed,
};

/*
 * End
 */

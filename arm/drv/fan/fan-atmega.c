
#include "drv/avr/fan.h"
#include "bios/fan.h"
#include "drv/fan.h"

static rv init(struct dev_fan *dev)
{
	return RV_OK;
}


static void set_speed(struct dev_fan *dev, uint8_t speed)
{
}


struct drv_fan drv_fan_avr = {
	.init = init,
	.set_speed = set_speed,
};

/*
 * End
 */

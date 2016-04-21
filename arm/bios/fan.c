
#include <stdint.h>

#include "bios/bios.h"
#include "bios/dev.h"
#include "bios/fan.h"
#include "drv/fan/fan.h"
#include "lib/atoi.h"


rv fan_init(struct dev_fan *dev)
{
	return eev->drv->init(dev);
}


void fan_init(void)
{
	dev_foreach(DEV_TYPE_FAN, fan_init_one, NULL);
}


void fan_set_speed(struct dev_fan *fan, uint8_t speed)
{
	fan->drv->set_speed(fan, speed);
}


/*
 * End
 */



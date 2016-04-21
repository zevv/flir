
#include <stdint.h>

#include "bios/bios.h"
#include "bios/printd.h"
#include "bios/power.h"
#include "drv/power/power.h"
#include "drv/power/power-dummy.h"


static rv init(struct dev_power *dev)
{
	return RV_OK;
}

static rv get_version(struct dev_power *power, char *buf, size_t maxlen)
{
	return RV_OK;
}


static rv set(struct dev_power *power, voltage U, current I)
{
	return RV_OK;
}


static rv get(struct dev_power *power, voltage *U, current *I)
{
	return RV_OK;
}


const struct drv_power drv_power_dummy = {
	.init = init,
	.get_version = get_version,
	.set = set,
	.get = get,
};


/*
 * End
 */

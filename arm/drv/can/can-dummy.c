
#include "bios/bios.h"
#include "bios/can.h"
#include "drv/can/can.h"
#include "drv/can/can-dummy.h"


static rv init(struct dev_can *can)
{
	return RV_OK;
}


static rv tx(struct dev_can *can, enum can_address_mode_t fmt, can_addr_t addr, const void *data, size_t len)
{
	return RV_OK;
}


const struct drv_can drv_can_dummy = {
	.init = init,
	.tx = tx
};



/*
 * End
 */

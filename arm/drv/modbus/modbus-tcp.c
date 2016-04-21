
#include <stdint.h>

#include "bios/bios.h"
#include "bios/printd.h"
#include "bios/modbus.h"
#include "drv/modbus/modbus.h"
#include "drv/modbus/modbus-tcp.h"


static rv init(struct dev_modbus *dev)
{
	return RV_OK;
}


static rv set_pin(struct dev_modbus *dev, uint8_t pin, uint8_t val)
{
	return RV_OK;
}


static rv get_pin(struct dev_modbus *dev, uint8_t pin, uint8_t *val)
{
	return RV_OK;
}


const struct drv_modbus drv_modbus_tcp = {
	.init = init,
	.set_pin = set_pin,
	.get_pin = get_pin,
};


/*
 * End
 */


#include <string.h>

#include "bios/dev.h"
#include "bios/bios.h"
#include "bios/printd.h"
#include "bios/cmd.h"
#include "bios/i2c.h"
#include "lib/atoi.h"
#include "drv/i2c/i2c.h"


rv i2c_init(struct dev_i2c *dev)
{
	return dev->drv->init(dev);
}


rv i2c_xfer(struct dev_i2c *dev, uint8_t addr, const void *txbuf, size_t txlen, void *rxbuf, size_t rxlen)
{
	rv r = dev->init_status;

	if(r == RV_OK) {
		r = dev->drv->xfer(dev, addr, txbuf, txlen, rxbuf, rxlen);
	}

	return r;
}


static rv on_cmd_i2c(uint8_t argc, char **argv)
{
	rv r = RV_EINVAL;

	if(argc >= 4u) {
		char cmd = argv[0][0];
		const struct dev_descriptor *dd = dev_find(argv[1], DEV_TYPE_I2C);

		if(dd != NULL) {

			uint8_t addr = a_to_s32(argv[2]);
			uint16_t reg = a_to_s32(argv[3]);

			if(cmd == 'r') {
				uint8_t len = a_to_s32(argv[4]);
				uint8_t buf[len];
				memset(buf, 0, len);

				i2c_xfer(dd->dev, addr, &reg, sizeof(reg), buf, len);

				hexdump(buf, len, addr);
				r = RV_OK;
			}
		} else {
			r = RV_ENODEV;
		}
	}

	return r;
}

CMD_REGISTER(i2c, on_cmd_i2c, "r[ead] <addr> <reg> <len> | w[write] <addr> <reg> <val...>");


/*
 * End
 */

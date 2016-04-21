
#include <stddef.h>
#include <string.h>

#include "bios/bios.h"
#include "bios/arch.h"
#include "bios/dev.h"
#include "bios/bios.h"
#include "bios/can.h"
#include "drv/can/can.h"
#include "bios/printd.h"
#include "bios/evq.h"
#include "bios/cmd.h"
#include "lib/atoi.h"



rv can_init(struct dev_can *dev)
{
	return dev->drv->init(dev);
}


rv can_set_speed(struct dev_can *dev, enum can_speed speed)
{
	rv r = RV_EIMPL;
	if(dev->drv->set_speed) {
		r = dev->drv->set_speed(dev, speed);
	}
	return r;
}


rv can_tx(struct dev_can *dev, enum can_address_mode_t fmt, can_addr_t addr, const void *data, size_t len)
{
	return dev->drv->tx(dev, fmt, addr, data, len);
}


rv can_get_stats(struct dev_can *dev, struct can_stats *stats)
{
	rv r;

	memset(stats, 0, sizeof(*stats));

	if(dev->drv->get_stats != NULL) {
		r = dev->drv->get_stats(dev, stats);
	} else {
		r = RV_EIMPL;
	}

	return r;
}


static void print_can(struct dev_descriptor *dd, void *ptr)
{
	struct dev_can *dev = dd->dev;
	struct can_stats s;

	rv r = can_get_stats(dev, &s);

	if(r == RV_OK) {
		printd("%s: %04x", dd->name, s.flags);
		printd(" rx total:%d err:%d xrun:%d", s.rx_total, s.rx_err, s.rx_xrun);
		printd(" tx total:%d err:%d xrun:%d", s.tx_total, s.tx_err, s.tx_xrun);
		printd("\n");
	}
}


static rv on_cmd_can(uint8_t argc, char **argv)
{
	rv r = RV_EINVAL;

	if(argc >= 1u) {

		char cmd = argv[0][0];

		if(cmd == 'l') {
			dev_foreach(DEV_TYPE_CAN, print_can, NULL);
			r = RV_OK;
		}

		if((cmd == 't') && (argc >= 3u)) {

			const struct dev_descriptor *dd = dev_find(argv[1], DEV_TYPE_CAN);

			if(dd != NULL) {

				can_addr_t addr = (can_addr_t)a_to_s32(argv[2]);
				uint8_t i;
				uint8_t len = argc - 3u;

				if(len <= 8u) {

					uint8_t data[len];
					for(i=0; i<len; i++) {
						int32_t v = (int32_t)a_to_s32(argv[i + 3u]);
						data[i] = (uint8_t)v;
					}

					r = can_tx(dd->dev, CAN_ADDRESS_MODE_STD, addr, data, len);
				} else {
					r = RV_EINVAL;
				}
				
			} else {
				r = RV_ENODEV;
			}
		}

	}

	return r;
}

CMD_REGISTER(can, on_cmd_can, "l[ist] | t[x] <dev> <addr> <data> ...");

/*
 * End
 */

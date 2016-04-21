
#include <stdio.h>
#include "bios/dev.h"
#include "bios/cmd.h"
#include "bios/bios.h"
#include "bios/printd.h"
#include "bios/eeprom.h"
#include "lib/atoi.h"
#include "drv/eeprom/eeprom.h"


rv eeprom_init(struct dev_eeprom *dev)
{
	return dev->drv->init(dev);
}


rv eeprom_read(struct dev_eeprom *eeprom, eeprom_addr_t addr, void *buf, size_t len)
{
	return eeprom->drv->read(eeprom, addr, buf, len);
}


rv eeprom_write(struct dev_eeprom *eeprom, eeprom_addr_t addr, const void *buf, size_t len)
{
	return eeprom->drv->write(eeprom, addr, buf, len);
}


eeprom_addr_t eeprom_get_size(struct dev_eeprom *eeprom)
{
	return eeprom->drv->get_size(eeprom);
}


static rv on_cmd_eeprom(uint8_t argc, char **argv)
{
	rv r = RV_EINVAL;

	if(argc >= 2u) {
		char cmd = argv[0][0];
		const struct dev_descriptor *dd = dev_find(argv[1], DEV_TYPE_EEPROM);

		if(dd != NULL) {
			if((cmd == 'r') && (argc >= 4u)) {

				eeprom_addr_t addr = (eeprom_addr_t)a_to_s32(argv[2]);
				size_t len = (size_t)a_to_s32(argv[3]);
				if(len > 32u) {
					len = 32u;
				}

				uint8_t buf[32];
				r = eeprom_read(dd->dev, addr, buf, len);
				printd("%02x %02x\n", buf[0], buf[1]);
				if(r == RV_OK) {
					hexdump(buf, len, addr);
				}
			}
			
			if((cmd == 'w') && (argc >= 4u)) {
				eeprom_addr_t addr = (eeprom_addr_t)a_to_s32(argv[2]);
				uint8_t val = (uint8_t)a_to_s32(argv[3]);
				r = eeprom_write(dd->dev, addr, &val, sizeof(val));
			}
			
			if(cmd == 't') {
				uint8_t buf[4];
				eeprom_read(dd->dev, 0, buf, 4);
				printd("%02x %02x %02x %02\n", buf[0], buf[1], buf[2], buf[3]);
				eeprom_read(dd->dev, 1, buf, 4);
				printd("%02x %02x %02x %02\n", buf[0], buf[1], buf[2], buf[3]);
			}

		} else {
			r = RV_ENODEV;
		}
	}

	return r;
}

CMD_REGISTER(eeprom, on_cmd_eeprom, "r <dev> <addr> <len> | w <dev> <addr> <val>");

/*
 * End
 */

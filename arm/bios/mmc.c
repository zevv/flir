
#include "bios/dev.h"
#include "bios/cmd.h"
#include "bios/bios.h"
#include "bios/printd.h"
#include "bios/mmc.h"
#include "lib/atoi.h"
#include "drv/mmc/mmc.h"


rv mmc_init(struct dev_mmc *dev)
{
	return dev->drv->init(dev);
}


rv mmc_read(struct dev_mmc *mmc, mmc_addr_t addr, void *buf, size_t len)
{
	return mmc->drv->read(mmc, addr, buf, len);
}


rv mmc_write(struct dev_mmc *mmc, mmc_addr_t addr, const void *buf, size_t len)
{
	return mmc->drv->write(mmc, addr, buf, len);
}


rv mmc_get_card_info(struct dev_mmc *mmc, struct mmc_card_info *info)
{
	return mmc->drv->get_card_info(mmc, info);
}


static void print_mmc(struct dev_descriptor *dd, void *ptr)
{
	struct dev_mmc *dev = dd->dev;
	struct mmc_card_info info;
	rv r = mmc_get_card_info(dev, &info);
	if(r == RV_OK) {
		const char *type = "?";
		if(info.type == MMC_CARD_V1) { type = "1"; }
		if(info.type == MMC_CARD_V2) { type = "2"; }
		if(info.type == MMC_CARD_V2_BLOCK) { type = "2b"; }
		if(info.type == MMC_CARD_V3) { type = "3"; }
		printd("%s: MMCv%s, %d Kb (%d sectors of %d bytes)\n", 
				dd->name, type, 
				(info.sector_count / 1024u) * info.block_size,
				info.sector_count, info.block_size);
	} else {
		printd("%s: %s\n", rv_str(r));
	}
}


static rv on_cmd_mmc(uint8_t argc, char **argv)
{
	rv r = RV_EINVAL;

	if(argc >= 1u) {
		char cmd = argv[0][0];

		if(cmd == 'l') {
			dev_foreach(DEV_TYPE_MMC, print_mmc, NULL);
			r = RV_OK;
		}
	}

	if(argc >= 2u) {
		char cmd = argv[0][0];
		const struct dev_descriptor *dd = dev_find(argv[1], DEV_TYPE_MMC);

		if(dd != NULL) {
			if((cmd == 'r') && (argc >= 4u)) {

				mmc_addr_t addr = (mmc_addr_t)a_to_s32(argv[2]);
				size_t len = (size_t)a_to_s32(argv[3]);
				uint8_t buf[512] = { 0 };
				r = mmc_read(dd->dev, addr, buf, len);
				hexdump(buf, len, addr);
			}
			
			if((cmd == 'w') && (argc >= 3u)) {
				mmc_addr_t addr = (mmc_addr_t)a_to_s32(argv[2]);
				uint8_t buf[512] = "Hello, world";
				r = mmc_write(dd->dev, addr, buf, sizeof(buf));
			}

		} else {
			r = RV_ENODEV;
		}
	}

	return r;
}

CMD_REGISTER(mmc, on_cmd_mmc, "l[ist] | r[ead] <dev> <addr> <len> | w[write] <dev> <addr> <val>");

/*
 * End
 */

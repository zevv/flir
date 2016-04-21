
#ifndef _lint

#include <stdint.h>
#include <stdio.h>

#include "bios/bios.h"
#include "bios/printd.h"
#include "bios/mmc.h"
#include "drv/mmc/mmc.h"
#include "drv/mmc/mmc-stdio.h"


static rv init(struct dev_mmc *dev)
{
	rv r = RV_OK;

	struct drv_mmc_stdio_data *dd = dev->drv_data;
	dd->_f = fopen(dd->fname, "r+b");

	if(dd->_f == NULL) {
		r = RV_ENOENT;
	}

	return r;
}


static rv readh(struct dev_mmc *dev, mmc_addr_t addr, void *buf, size_t len)
{
	struct drv_mmc_stdio_data *dd = dev->drv_data;
	if(dd->_f) {
		fseek(dd->_f, addr * 512, SEEK_SET);
		fread(buf, 1, len, dd->_f);
	}
	return RV_OK;
}


static rv writeh(struct dev_mmc *dev, mmc_addr_t addr, const void *buf, size_t len)
{
	struct drv_mmc_stdio_data *dd = dev->drv_data;
	if(dd->_f) {
		fseek(dd->_f, addr * 512, SEEK_SET);
		fwrite(buf, 1, len, dd->_f);
	}
	return RV_OK;
}


static rv get_card_info(struct dev_mmc *dev, struct mmc_card_info *info)
{
	struct drv_mmc_stdio_data *dd = dev->drv_data;
	info->sector_count = dd->size / 512;
	return RV_OK;
}


const struct drv_mmc drv_mmc_stdio = {
	.init = init,
	.read = readh,
	.write = writeh,
	.get_card_info = get_card_info,
};


#endif

/*
 * End
 */

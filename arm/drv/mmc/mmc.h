#ifndef drv_mmc_h
#define drv_mmc_h

#include "config.h"
#include "bios/mmc.h"

#ifdef DRV_MMC
#define DEV_INIT_MMC(name, drv, ...) DEV_INIT(MMC, mmc, name, drv,__VA_ARGS__) 
#else
#define DEV_INIT_MMC(name, drv, ...)
#endif

struct dev_mmc {
	const struct drv_mmc *drv;
	void *drv_data;
	rv init_status;
};

struct drv_mmc {
	rv (*init)(struct dev_mmc *mmc);
	rv (*read)(struct dev_mmc *mmc, mmc_addr_t addr, void *buf, size_t len);
	rv (*write)(struct dev_mmc *mmc, mmc_addr_t addr, const void *buf, size_t len);
	rv (*get_card_info)(struct dev_mmc *mmc, struct mmc_card_info *info);
};

#endif

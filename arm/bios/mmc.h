#ifndef bios_mmc_h
#define bios_mmc_h

#include <stdint.h>
#include <stddef.h>

#include "bios/bios.h"

typedef enum {
	MMC_STATUS_IDLE,
	MMC_STATUS_DETECTING,
	MMC_STATUS_READY,
	MMC_STATUS_ERROR,
} mmc_status;


typedef enum {
	MMC_CARD_UNKNOWN,
	MMC_CARD_V1,
	MMC_CARD_V2,
	MMC_CARD_V2_BLOCK,
	MMC_CARD_V3
} mmc_card_type;

struct mmc_card_info {
	mmc_card_type type;
	uint16_t block_size;
	uint32_t sector_count;
};

typedef uint32_t mmc_addr_t;

struct dev_mmc;

rv mmc_init(struct dev_mmc *dev);
rv mmc_read(struct dev_mmc *mmc, mmc_addr_t addr, void *buf, size_t len);
rv mmc_write(struct dev_mmc *mmc, mmc_addr_t addr, const void *buf, size_t len);
rv mmc_get_card_info(struct dev_mmc *mmc, struct mmc_card_info *info);

#endif

#ifndef drv_mmc_spi_h
#define drv_mmc_spi_h

#include "drv/mmc/mmc.h"
#include "bios/mmc.h"
#include "bios/spi.h"

struct drv_mmc_spi_data {
	mmc_addr_t size;
	struct dev_gpio *dev_gpio_cs;
	gpio_pin cs_gpio_pin;
	
	struct dev_gpio *dev_gpio_detect;
	gpio_pin detect_gpio_pin;

	struct dev_spi *dev_spi;

	struct mmc_card_info _card_info;
	mmc_status _status;
	uint8_t _debug;
};

extern const struct drv_mmc drv_mmc_spi;

#endif



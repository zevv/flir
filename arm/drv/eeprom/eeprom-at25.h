#ifndef drv_at25_eeprom_h
#define drv_at25_eeprom_h

#include "drv/eeprom/eeprom.h"

struct drv_eeprom_at25_data {
	eeprom_addr_t size;
	struct dev_gpio *dev_gpio_cs;
	gpio_pin cs_gpio_pin;
	struct dev_spi *dev_spi;
};

extern struct drv_eeprom drv_eeprom_at25;

#endif


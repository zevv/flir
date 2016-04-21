#ifndef drv_eeprom_h
#define drv_eeprom_h

#include "config.h"
#include "bios/eeprom.h"

#ifdef DRV_EEPROM
#define DEV_INIT_EEPROM(name, drv, ...) DEV_INIT(EEPROM, eeprom, name, drv,__VA_ARGS__) 
#else
#define DEV_INIT_EEPROM(name, drv, ...)
#endif

struct dev_eeprom {
	const struct drv_eeprom *drv;
	void *drv_data;
	rv init_status;
};

struct drv_eeprom {
	rv (*init)(struct dev_eeprom *eeprom);
	rv (*read)(struct dev_eeprom *eeprom, eeprom_addr_t addr, void *buf, size_t len);
	rv (*write)(struct dev_eeprom *eeprom, eeprom_addr_t addr, const void *buf, size_t len);
	eeprom_addr_t (*get_size)(struct dev_eeprom *eeprom);
};

#endif

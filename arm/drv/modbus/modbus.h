#ifndef drv_modbus_h
#define drv_modbus_h

#include "config.h"
#include "bios/modbus.h"

#ifdef DRV_MODBUS
#define DEV_INIT_MODBUS(name, drv, ...) DEV_INIT(MODBUS, modbus, name, drv,__VA_ARGS__) 
#else
#define DEV_INIT_MODBUS(name, drv, ...)
#endif

struct dev_modbus {
	const struct drv_modbus *drv;
	void *drv_data;
	rv init_status;
};

struct drv_modbus {
	rv (*init)(struct dev_modbus *dev);
	rv (*set_pin)(struct dev_modbus *modbus, uint8_t pin, uint8_t val);
	rv (*get_pin)(struct dev_modbus *modbus, uint8_t pin, uint8_t *val);
};

#endif

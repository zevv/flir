#ifndef drv_i2c_h
#define drv_i2c_h

#include "config.h"
#include "bios/i2c.h"

#ifdef DRV_I2C
#define DEV_INIT_I2C(name, drv, ...) DEV_INIT(I2C, i2c, name, drv,__VA_ARGS__) 
#else
#define DEV_INIT_I2C(name, drv, ...)
#endif

struct dev_i2c {
	const struct drv_i2c *drv;
	void *drv_data;
	rv init_status;
};

struct drv_i2c {
	rv (*init)(struct dev_i2c *i2c);
	rv (*xfer)(struct dev_i2c *i2c, uint8_t addr, const void *txbuf, size_t txlen, void *rxbuf, size_t rxlen);
};

#endif

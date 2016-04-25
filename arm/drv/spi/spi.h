#ifndef drv_spi_h
#define drv_spi_h

#include "config.h"
#include "bios/spi.h"

#ifdef DRV_SPI
#define DEV_INIT_SPI(name, drv, ...) DEV_INIT(SPI, spi, name, drv,__VA_ARGS__) 
#else
#define DEV_INIT_SPI(name, drv, ...)
#endif

struct dev_spi {
	const struct drv_spi *drv;
	void *drv_data;
	rv init_status;
};

struct drv_spi {
	rv (*init)(struct dev_spi *spi);
	rv (*read)(struct dev_spi *spi, void *buf, size_t len);
	rv (*read_async)(struct dev_spi *spi, void *buf, size_t len, void(*fn)(void));
	rv (*write)(struct dev_spi *spi, const void *buf, size_t len);
	rv (*read_write)(struct dev_spi *spi, void *buf, size_t len);
};

#endif

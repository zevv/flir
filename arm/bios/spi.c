
#include "bios/dev.h"
#include "bios/bios.h"
#include "bios/spi.h"
#include "drv/spi/spi.h"


rv spi_init(struct dev_spi *dev)
{
	return dev->drv->init(dev);
}


rv spi_read(struct dev_spi *dev, void *buf, size_t len)
{
	rv r = dev->init_status;

	if(r == RV_OK) {
		r = dev->drv->read(dev, buf, len);
	}

	return r;
}


rv spi_read_async(struct dev_spi *dev, void *buf, size_t len, void(*fn)(void))
{
	rv r = dev->init_status;

	if(r == RV_OK) {
		r = dev->drv->read_async(dev, buf, len, fn);
	}

	return r;
}


rv spi_write(struct dev_spi *dev, const void *buf, size_t len)
{
	rv r = dev->init_status;

	if(r == RV_OK) {
		r = dev->drv->write(dev, buf, len);
	}

	return r;
}


rv spi_read_write(struct dev_spi *dev, void *buf, size_t len)
{
	rv r = dev->init_status;

	if(r == RV_OK) {
		r = dev->drv->read_write(dev, buf, len);
	}

	return r;
}



/*
 * End
 */

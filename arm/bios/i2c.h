#ifndef bios_i2c_h
#define bios_i2c_h

#include <stdint.h>
#include <stddef.h>

struct dev_i2c;

rv i2c_init(struct dev_i2c *dev);
rv i2c_xfer(struct dev_i2c *dev, uint8_t addr, const void *txbuf, size_t txlen, void *rxbuf, size_t rxlen);

#endif

#ifndef bios_spi_h
#define bios_spi_h

#include <stdint.h>
#include <stddef.h>

struct dev_spi;

rv spi_init(struct dev_spi *dev);
rv spi_read(struct dev_spi *dev, void *buf, size_t len);
rv spi_write(struct dev_spi *dev, const void *buf, size_t len);
rv spi_read_write(struct dev_spi *dev, void *buf, size_t len);

#endif

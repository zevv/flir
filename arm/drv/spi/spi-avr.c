
/*
 * Driver for EEPROM connected to SPI bus
 */

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "bios/bios.h"
#include "bios/printd.h"
#include "drv/spi/spi-avr.h"

static rv init(struct dev_spi *dev)
{
	/* Enable SPI in master mode, set SCK and MOSI to output 
	 * PB4 functions as the chip select, and should be set to output mode
	 * to be able to use the SPI in master mode. */

	DDRB |= (1<<PB4) | (1<<PB5) | (1<<PB6);
	SPCR = (1<<SPE) | (1<<MSTR);

	return RV_OK;
}


static rv read_write(struct dev_spi *dev, void *buf, size_t len)
{
	uint8_t *p = buf;

	uint8_t s = SREG; cli();

	while(len--) {
		SPDR = *p;
		while(!(SPSR &(1<<SPIF)));
		*p++ = SPDR;
	}

	SREG = s;

	return RV_OK;
}


static rv writeh(struct dev_spi *dev, const void *buf, size_t len)
{
	const uint8_t *p = buf;

	uint8_t s = SREG; cli();

	while(len--) {
		SPDR = *p++;
		while(!(SPSR &(1<<SPIF)));
	}

	SREG = s;

	return RV_OK;
}


struct drv_spi drv_spi_avr = {
	.init = init,
	.read_write = read_write,
	.write = writeh,
};

/*
 * End
 */

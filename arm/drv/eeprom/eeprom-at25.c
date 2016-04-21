
/*
 * Driver for EEPROM connected to SPI bus
 */

#include <stdint.h>

#include "bios/bios.h"
#include "bios/spi.h"
#include "bios/printd.h"
#include "bios/gpio.h"
#include "drv/eeprom/eeprom-at25.h"

/* Atmel AT25256B Commands */

#define EEPROM_WREN     0x06u  /* Set Write Enable Latch */
#define EEPROM_WRDI     0x04u  /* Reset Write Enable latch */
#define EEPROM_RDSR     0x05u  /* Read Status Register */
#define EEPROM_WRSR     0x01u  /* Write Status register */
#define EEPROM_READ     0x03u  /* Read Data from Memory */
#define EEPROM_WRITE    0x02u  /* Write Data to memory */

/* Atmel AT25256B Status Registers */

#define EEPROM_NRDY     0x01u  /* 0 = Ready, 1 = Write Cycle in progress */
#define EEPROM_WEN      0x02u  /* 0 = Not WR-enabled, 1= WR-enabled */
#define EEPROM_BP0      0x04u  /* Block Procection 0 */
#define EEPROM_BP1      0x08u  /* Block protection 1 */
#define EEPROM_WPEN     0x80u  /* Write Protect Pin Enable */

#define PAGE_SIZE 64

static void _delay(uint8_t n)
{
	volatile uint8_t d = n;
	while(d > 0u) {
		d--;
	}
}


static void set_cs(struct drv_eeprom_at25_data *dd, uint8_t v)
{
	if(v != 0u) {
		_delay(5);
		(void)gpio_set_pin(dd->dev_gpio_cs, dd->cs_gpio_pin, 1);
		_delay(5);
	} else {
		_delay(5);
		(void)gpio_set_pin(dd->dev_gpio_cs, dd->cs_gpio_pin, 0);
		_delay(5);
	}
}


static void set_wren(struct drv_eeprom_at25_data *dd, uint8_t v)
{
	uint8_t cmd = (v != 0u) ? EEPROM_WREN : EEPROM_WRDI;
	set_cs(dd, 0);
	spi_write(dd->dev_spi, &cmd, sizeof(cmd));
	set_cs(dd, 1);
}


static uint8_t read_status(struct drv_eeprom_at25_data *dd)
{
	uint8_t v[2] = { EEPROM_RDSR, 0x00 };

	set_cs(dd, 0);
	spi_read_write(dd->dev_spi, &v, sizeof(v));
	set_cs(dd, 1);

	return v[1];
}


static rv init(struct dev_eeprom *dev)
{
	struct drv_eeprom_at25_data *dd = dev->drv_data;

	(void)gpio_set_pin_direction(dd->dev_gpio_cs, dd->cs_gpio_pin, GPIO_DIR_OUTPUT);
	set_cs(dd, 1);

	return RV_OK;
}


static rv readh(struct dev_eeprom *dev, eeprom_addr_t addr, void *buf, size_t len)
{
	struct drv_eeprom_at25_data *dd = dev->drv_data;

	uint8_t cmd[3];
	cmd[0] = EEPROM_READ;
	cmd[1] = (uint8_t)(addr >> 8);
	cmd[2] = (uint8_t)(addr & 0xFFu);

	set_cs(dd, 0);
	spi_write(dd->dev_spi, cmd, sizeof(cmd));
	spi_read(dd->dev_spi, buf, len);
	set_cs(dd, 1);

	return RV_OK;
}


static rv writeh(struct dev_eeprom *dev, eeprom_addr_t addr, const void *buf, size_t len)
{
	struct drv_eeprom_at25_data *dd = dev->drv_data;

	while(len > 0) {

		/* handle page boundaries */

                eeprom_addr_t addr_next = (addr / PAGE_SIZE + 1) * PAGE_SIZE;
                size_t len_max = addr_next - addr;
                size_t l = (len > len_max) ? len_max : len;

		/* Write enable */

		set_wren(dd, 1);

		/* perform page write */

		uint8_t cmd[3];
		cmd[0] = EEPROM_WRITE;
		cmd[1] = (uint8_t)(addr >> 8);
		cmd[2] = (uint8_t)(addr & 0xFFu);
	
		set_cs(dd, 0);
		spi_write(dd->dev_spi, cmd, sizeof(cmd));
		spi_write(dd->dev_spi, buf, l);
		set_cs(dd, 1);

		while((read_status(dd) & EEPROM_NRDY) != 0u) {
			/* spin */
		}

		addr += l;
		buf += l;
		len -= l;
	}

	set_wren(dd, 0);

	return RV_OK;
}


static eeprom_addr_t get_size(struct dev_eeprom *dev)
{
	return 0;
}


struct drv_eeprom drv_eeprom_at25 = {
	.init = init,
	.read = readh,
	.write = writeh,
	.get_size = get_size
};

/*
 * End
 */

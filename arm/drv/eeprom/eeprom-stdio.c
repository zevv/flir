
#ifndef _lint

#include <stdint.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include "bios/bios.h"
#include "bios/eeprom.h"
#include "drv/eeprom/eeprom.h"
#include "drv/eeprom/eeprom-stdio.h"


static rv init(struct dev_eeprom *dev)
{
	struct drv_eeprom_stdio_data *dd = dev->drv_data;
	dd->_f = fopen(dd->fname, "r+b");
	if(dd->_f == NULL) dd->_f = fopen(dd->fname, "w+b");
	assert(dd->_f);
	return RV_OK;
}


static rv readh(struct dev_eeprom *dev, eeprom_addr_t addr, void *buf, size_t len)
{
	struct drv_eeprom_stdio_data *dd = dev->drv_data;
	memset(buf, 0xff, len);
	fseek(dd->_f, addr, SEEK_SET);
	fread(buf, 1, len, dd->_f);
	return RV_OK;
}


static rv writeh(struct dev_eeprom *dev, eeprom_addr_t addr, const void *buf, size_t len)
{
	struct drv_eeprom_stdio_data *dd = dev->drv_data;
	fseek(dd->_f, addr, SEEK_SET);
	fwrite(buf, 1, len, dd->_f);
	fflush(dd->_f);
	return RV_OK;
}


eeprom_addr_t get_size(struct dev_eeprom *dev)
{
	struct drv_eeprom_stdio_data *dd = dev->drv_data;
	return dd->size;
}


const struct drv_eeprom drv_eeprom_stdio = {
	.init = init,
	.read = readh,
	.write = writeh,
	.get_size = get_size
};

#endif

/*
 * End
 */

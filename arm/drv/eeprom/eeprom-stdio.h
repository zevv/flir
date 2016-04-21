#ifndef drv_eeprom_stdio_h
#define drv_eeprom_stdio_h

#include <stdio.h>

#include "drv/eeprom/eeprom.h"

struct drv_eeprom_stdio_data {
	const char *fname;
	eeprom_addr_t size;

	FILE *_f;
};

extern const struct drv_eeprom drv_eeprom_stdio;

#endif


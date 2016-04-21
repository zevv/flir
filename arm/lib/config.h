#ifndef lib_config_h
#define lib_config_h

#include <stdint.h>
#include <stddef.h>
#include "bios/bios.h"
#include "bios/eeprom.h"

rv config_init(struct dev_eeprom *dev, eeprom_addr_t addr_start, eeprom_addr_t addr_end);

rv config_write(const char *id, void *data, size_t data_len);
rv config_read(const char *id, void *data, size_t data_len);

#endif

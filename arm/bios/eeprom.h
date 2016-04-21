#ifndef bios_eeprom_h
#define bios_eeprom_h

#include <stdint.h>
#include <stddef.h>

#include "bios/bios.h"

typedef uint32_t eeprom_addr_t;

struct dev_eeprom;

rv eeprom_init(struct dev_eeprom *dev);
rv eeprom_read(struct dev_eeprom *eeprom, eeprom_addr_t addr, void *buf, size_t len);
rv eeprom_write(struct dev_eeprom *eeprom, eeprom_addr_t addr, const void *buf, size_t len);
eeprom_addr_t eeprom_get_size(struct dev_eeprom *eeprom);

#endif

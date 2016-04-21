#ifndef lib_canopen_analyzer_h
#define lib_canopen_analyzer_h

#include "config.h"

#ifdef LIB_CANOPEN_ANALYZER
void canalyzer_dump(const char *prefix, struct dev_can *dev, enum can_address_mode_t fmt, 
		can_addr_t addr, const uint8_t *data, size_t len);
#else
__attribute__ (( __used__ )) static void canalyzer_dump(const char *prefix, struct dev_can *dev, enum can_address_mode_t fmt, 
		can_addr_t addr, const uint8_t *data, size_t len) { }
#endif

#endif

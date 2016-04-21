#ifndef bios_log_h
#define bios_log_h

#include <stddef.h>

enum event_type { 
	LOG_BOOT = 1,
	LOG_FLASH = 2,
	LOG_EEPROM_INIT = 3,
};

void log_init(void);

void log_begin(enum event_type ev);
void log_U(voltage U);
void log_commit(void);


#endif

#ifndef bios_printd_h
#define bios_printd_h

#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>

void vfprintd(rv (*tx)(uint8_t c), const char *fmt, va_list va);
void vprintd(const char *fmt, va_list va);
void printd(const char *fmt, ...); 
void hexdump(const void *addr, size_t len, uintptr_t offset);
void fhexdump(rv (*tx)(uint8_t c), const void *addr, size_t len, uintptr_t offset);
void fprintd(rv (*tx)(uint8_t c), const char *fmt, ...);


//__attribute__((format(printf, 1, 2)));

#endif

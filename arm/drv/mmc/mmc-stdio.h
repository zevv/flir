#ifndef drv_mmc_stdio_h
#define drv_mmc_stdio_h

#include <stdio.h>

#include "drv/mmc/mmc.h"

struct drv_mmc_stdio_data {
	const char *fname;
	mmc_addr_t size;

	FILE *_f;
};

extern const struct drv_mmc drv_mmc_stdio;

#endif


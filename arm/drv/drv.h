#ifndef drv_drv_h
#define drv_drv_h

#include "config.h"
#include "bios/dev.h"

/*
 * These macros are used to define device instances in the platform definition
 * files. DEV_INIT() is called by the DEV_INIT_XXX() macros which are defined
 * in the various driver headers, and allocates two global structs:
 *
 * - a 'dev_descriptor' struct is placed in a specific section 'dev' and can be
 *   iterated to find all devices of a given type using the dev_foreach() functions
 *   from bios/dev.h
 *
 * - a 'dev_xxx' struct is globally defined and must be used as opaque instance
 *   pointers when calling the various bios functions for handling the device.
 */

#define DEV_INIT(dt, st, n, d, ...)                               \
	static struct d ## _data dev_ ## n ## _data =  {          \
		__VA_ARGS__                                       \
	};                                                        \
	struct dev_ ## st n = {                                   \
		.drv = &d,                                        \
		.drv_data = &dev_ ## n ## _data,                  \
		.init_status = RV_ENOTREADY,                      \
	};                                                        \
	static const struct dev_descriptor dev_ ## n              \
	__attribute__ ((__section__("dev")))                      \
	__attribute__ ((__used__))                                \
	__attribute__ ((aligned(1)))                              \
        = {                                                       \
		.name = #n,                                       \
		.dev = &n,                                        \
		.type = DEV_TYPE_ ## dt                           \
	};


#endif

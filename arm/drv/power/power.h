#ifndef drv_power_h
#define drv_power_h

#include "config.h"
#include "bios/power.h"

#ifdef DRV_POWER
#define DEV_INIT_POWER(name, drv, ...) DEV_INIT(POWER, power, name, drv,__VA_ARGS__) 
#else
#define DEV_INIT_POWER(name, drv, ...)
#endif

struct dev_power {
	const struct drv_power *drv;
	void *drv_data;
	rv init_status;
};

struct drv_power {
	rv (*init)(struct dev_power *dev);
	rv (*get_version)(struct dev_power *power, char *buf, size_t maxlen);
	rv (*set)(struct dev_power *power, voltage U, current I);
	rv (*get)(struct dev_power *power, voltage *U, current *I);
};

#endif

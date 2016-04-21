#ifndef drv_fan_h
#define drv_fan_h

#include "config.h"
#include "bios/fan.h"

#ifdef DRV_FAN
#define DEV_INIT_FAN(name, drv, ...) DEV_INIT(FAN, fan, name, drv,__VA_ARGS__) 
#else
#define DEV_INIT_FAN(name, drv, ...)
#endif

struct dev_fan {
	const struct drv_fan *drv;
	void *drv_data;
	rv init_status;
	uint8_t speed;
};

struct drv_fan {
	rv (*init)(struct dev_fan *fan);
	void (*set_speed)(struct dev_fan *fan, uint8_t speed);
};

#endif

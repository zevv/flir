#ifndef drv_can_h
#define drv_can_h

#include "config.h"
#include "bios/can.h"

#ifdef DRV_CAN
#define DEV_INIT_CAN(name, drv, ...) DEV_INIT(CAN, can, name, drv, __VA_ARGS__) 
#else
#define DEV_INIT_CAN(name, drv, ...) 
#endif

struct dev_can {
	const struct drv_can *drv;
	void *drv_data;
	rv init_status;
};

struct drv_can {
	rv (*init)(struct dev_can *can);
	rv (*set_speed)(struct dev_can *can, enum can_speed speed);
	rv (*tx)(struct dev_can *can, enum can_address_mode_t fmt, can_addr_t addr, const void *data, size_t len);
	rv (*get_stats)(struct dev_can *dev, struct can_stats *stats);
};

#endif

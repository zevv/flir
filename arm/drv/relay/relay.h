#ifndef drv_relay_h
#define drv_relay_h

#include "config.h"
#include "bios/relay.h"

#ifdef DRV_RELAY
#define DEV_INIT_RELAY(name, drv, ...) DEV_INIT(RELAY, relay, name, drv, __VA_ARGS__) 
#else
#define DEV_INIT_RELAY(name, drv, ...)
#endif

struct dev_relay {
	const struct drv_relay *drv;
	void *drv_data;
	rv init_status;
	enum relay_state_t _state;
};

struct drv_relay {
	rv (*init)(struct dev_relay *relay);
	rv (*set)(struct dev_relay *relay, enum relay_state_t onoff);
};

#endif

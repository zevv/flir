#ifndef drv_led_h
#define drv_led_h

#include "config.h"
#include "bios/led.h"

#ifdef DRV_LED
#define DEV_INIT_LED(name, drv, ...) DEV_INIT(LED, led, name, drv,__VA_ARGS__) 
#else
#define DEV_INIT_LED(name, drv, ...)
#endif

struct dev_led {
	const struct drv_led *drv;
	void *drv_data;
	rv init_status;
	enum led_state_t state;
	rv init_state;
};

struct drv_led {
	rv (*init)(struct dev_led *led);
	rv (*set)(struct dev_led *led, uint8_t onoff);
};

#endif

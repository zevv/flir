#ifndef drv_button_h
#define drv_button_h

#include "config.h"
#include "bios/button.h"

#ifdef DRV_BUTTON
#define DEV_INIT_BUTTON(name, drv, ...) DEV_INIT(BUTTON, button, name, drv, __VA_ARGS__) 
#else
#define DEV_INIT_BUTTON(name, drv, ...)
#endif

struct dev_button {
	const struct drv_button *drv;
	void *drv_data;
	rv init_status;
	uint8_t previous_state;
	uint8_t current_state;
	uint8_t counter;
};

struct drv_button {
	rv (*init)(struct dev_button *button);
	enum button_state (*get)(struct dev_button *button);
};

#endif


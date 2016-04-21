
#ifndef drv_led_linux_h
#define drv_led_linux_h

#include "drv/led/led.h"

extern const struct drv_led drv_led_linux;

struct drv_led_linux_data {
	int y;
	int x;
	uint32_t color;
	int onoff;
};

#endif

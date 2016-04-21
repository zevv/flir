
#include <stdio.h>
#include <stdint.h>

#include <SDL/SDL.h>

#include "bios/bios.h"
#include "bios/led.h"
#include "drv/led/led.h"
#include "drv/led/led-linux.h"
#include "bios/printd.h"
#include "plat.h"

extern SDL_Surface *screen;

static rv init(struct dev_led *led)
{
	return RV_OK;
}


static rv set(struct dev_led *dev, uint8_t onoff)
{
	struct drv_led_linux_data *dd = dev->drv_data;

	if(dd && screen) {
		SDL_Rect r;
		r.x = dd->x * 10;
		r.y = dd->y * 10;
		r.w = 8;
		r.h = 8;

		uint32_t color = onoff ? dd->color : (dd->color & 0x3f3f3f);
		SDL_FillRect(screen, &r, color);
		SDL_Flip(screen);
	}

	return RV_OK;
}


const struct drv_led drv_led_linux = {
	.init = init,
	.set = set,
};

/*
 * End
 */

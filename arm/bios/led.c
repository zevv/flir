
#include <stdint.h>
#include <string.h>

#include "bios/bios.h"
#include "bios/dev.h"
#include "bios/evq.h"
#include "bios/printd.h"
#include "bios/cmd.h"
#include "bios/led.h"
#include "drv/led/led.h"
#include "lib/atoi.h"


static uint8_t blink = 0;

rv led_init(struct dev_led *dev)
{
	rv r = dev->drv->init(dev);
	if(r == RV_OK) {
		r = led_set(dev, LED_STATE_OFF);
	}
	return r;
}


rv led_set(struct dev_led *dev, enum led_state_t state)
{
	rv r = dev->init_state;
	
	if(r == RV_OK) {

		dev->state = state;
		uint8_t v = 0;

		switch(state) {
			case LED_STATE_OFF:
				v = 0;
				break;
			case LED_STATE_ON:
				v = 1;
				break;
			case LED_STATE_BLINK:
				v = blink;
				break;
			default:
				/* empty */
				break;

		}

		r =  dev->drv->set(dev, v);
	}

	return r;
}


static void on_led_blink(struct dev_descriptor *dd, void *ptr)
{
	struct dev_led *dev = dd->dev;
	if(dev->state == LED_STATE_BLINK) {
		(void)dev->drv->set(dev, blink);
	}
}


static void on_ev_tick_10hz(event_t *ev, void *data)
{
	blink = 1u-blink;
	dev_foreach(DEV_TYPE_LED, on_led_blink, NULL);
}

EVQ_REGISTER(EV_TICK_10HZ, on_ev_tick_10hz);


static void print_led(struct dev_descriptor *dd, void *ptr)
{
	const struct dev_led *dev = dd->dev;
	printd("%s: %d\n", dd->name, dev->state);
}


static rv on_cmd_led(uint8_t argc, char **argv)
{
	rv r = RV_EINVAL;

	if(argc >= 1u) {
		char cmd = argv[0][0];

		if(cmd == 'l') {
			dev_foreach(DEV_TYPE_LED, print_led, NULL);
			r = RV_OK;
		}

		if((cmd == 's') && (argc >= 3u)) {
			const struct dev_descriptor *dd = dev_find(argv[1], DEV_TYPE_LED);
			if(dd != NULL) {
				char *s = argv[2];
				if(strcmp(s, "off") == 0) {
					r = led_set(dd->dev, LED_STATE_OFF);
				} else if(strcmp(s, "on") == 0) {
					r = led_set(dd->dev, LED_STATE_ON);
				} else if(strcmp(s, "blink") == 0) {
					r = led_set(dd->dev, LED_STATE_BLINK);
				} else {
					r = RV_EINVAL;
				}
			} else {
				r = RV_ENODEV;
			}
		}
	}
	return r;
}

CMD_REGISTER(led, on_cmd_led, "l[ist] | s[et] <id> [off|on|blink]>");


/*
 * End
 */



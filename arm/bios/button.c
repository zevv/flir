
#include <assert.h>

#include "bios/bios.h"
#include "bios/dev.h"
#include "bios/evq.h"
#include "bios/printd.h"
#include "bios/cmd.h"
#include "bios/button.h"
#include "drv/button/button.h"
#include <string.h>


#define DEBOUNCING_FILTER 10u



static enum button_state state = BUTTON_STATE_RELEASE;


rv button_init(struct dev_button *dev)
{
	return dev->drv->init(dev);
}



static void on_ev_button(event_t *ev, void *data)
{
//	state = ev->button.state;
}

EVQ_REGISTER(EV_BUTTON, on_ev_button);


static void on_scan_button(struct dev_descriptor *dd, void *ptr)
{
	struct dev_button *dev = dd->dev;
	assert(dev->drv->get);
	state = dev->drv->get(dev);

	if ( state == BUTTON_STATE_PUSH ){
		if (dev->counter < DEBOUNCING_FILTER){ 
			dev->counter++;
		}
		else{
			dev->current_state = 1;
		}
	}
	else{
		if (dev->counter > 0u){
			dev->counter--;
		}
		else{
			dev->current_state = 0;
		}
	}

	if ( (dev->current_state != dev->previous_state) ){
		if (dev->current_state == 1u){
			event_t ev;
			ev.type = EV_BUTTON;
			ev.button.dev = dev;
			ev.button.state = BUTTON_STATE_PUSH;
			evq_push(&ev);
		}
	}
	dev->previous_state = dev->current_state;
	dev->current_state = 0; 
}


static void on_ev_tick_100hz(event_t *ev, void *data)
{
	dev_foreach(DEV_TYPE_BUTTON, on_scan_button, NULL);
}

EVQ_REGISTER(EV_TICK_100HZ, on_ev_tick_100hz);


static rv on_cmd_button(uint8_t argc, char **argv)
{
	rv r = RV_EINVAL;

	if(argc >= 2u) {
		char cmd = argv[0][0];


		if((cmd == 'p') || (cmd == 'r')) {

			const struct dev_descriptor *dd = dev_find(argv[1], DEV_TYPE_BUTTON);

			if(dd != NULL) {

				event_t ev;
				ev.type = EV_BUTTON;
				ev.button.dev = dd->dev;
				if(cmd == 'p') {
					ev.button.state = BUTTON_STATE_PUSH;
				}
				if(cmd == 'r') {
					ev.button.state = BUTTON_STATE_RELEASE;
				}
				evq_push(&ev);
				r = RV_OK;
			} else {
				r = RV_ENODEV;
			}
		}
	}

	return r;
}

CMD_REGISTER(button, on_cmd_button, "p[ush] <id> | r[elease] <id>");

/*
 * End
 */


#include <string.h>

#include "bios/bios.h"
#include "bios/printd.h"
#include "bios/evq.h"
#include "bios/led.h"
#include "bios/can.h"

#include "lib/config.h"



static void on_ev_boot(event_t *ev, void *data)
{
	(void)led_set(&led_green, LED_STATE_ON);
	(void)led_set(&led_yellow, LED_STATE_BLINK);
	printd("BLINK\n");

}


EVQ_REGISTER(EV_BOOT, on_ev_boot);

static void on_ev_tick(event_t *ev, void *data)
{
	printd("click\n");
}

EVQ_REGISTER(EV_BUTTON, on_ev_tick);



/*
 * End
 */

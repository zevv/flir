
#include <string.h>

#include "bios/bios.h"
#include "bios/dev.h"
#include "bios/printd.h"

#include "bios/cmd.h"
#include "bios/relay.h"
#include "drv/relay/relay.h"


rv relay_init(struct dev_relay *dev)
{
	rv r = dev->drv->init(dev);
	if(r == RV_OK) {
		r = relay_set(dev, RELAY_STATE_OPEN);
	}
	return r;
}


rv relay_set(struct dev_relay *relay, enum relay_state_t state)
{
	relay->_state = state;
	return relay->drv->set(relay, state);
}


rv relay_get(struct dev_relay *relay, enum relay_state_t *state)
{
	*state = relay->_state;
	return RV_OK;
}


static void print_relay(struct dev_descriptor *dd, void *ptr)
{
	const struct dev_relay *dev = dd->dev;
	printd("%s: %d\n", dd->name, dev->_state);
}

static rv on_cmd_relay(uint8_t argc, char **argv)
{
	rv r = RV_EINVAL;

	if(argc >= 1u) {
		char cmd = argv[0][0];

		if(cmd == 'l') {
			dev_foreach(DEV_TYPE_RELAY, print_relay, NULL);
			r = RV_OK;
		}

		if((cmd == 's') && (argc >= 3u)) {
			const struct dev_descriptor *dd = dev_find(argv[1], DEV_TYPE_RELAY);
			if(dd != NULL) {
				char *s = argv[2];
				if(strcmp(s, "off") == 0) {
					r = relay_set(dd->dev, RELAY_STATE_OPEN);
				} else if(strcmp(s, "on") == 0) {
					r = relay_set(dd->dev, RELAY_STATE_CLOSED);
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

CMD_REGISTER(relay, on_cmd_relay, "l[ist] | s[et] <name> <on|off>");


/*
 * End
 */

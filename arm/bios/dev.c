#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "bios/dev.h"
#include "bios/cmd.h"
#include "bios/printd.h"


void dev_foreach(enum dev_type type, dev_iterator cb, void *ptr)
{
#ifndef _lint
	extern struct dev_descriptor __start_dev;
	extern struct dev_descriptor __stop_dev;
	struct dev_descriptor *dd = &__start_dev;

	while(dd < &__stop_dev) {
		if((type == DEV_TYPE_ANY) || (dd->type == type)) {
			cb(dd, ptr);
		}
		dd++;
	}
#endif
}


struct dev_descriptor *dev_find(const char *name, enum dev_type type)
{
	struct dev_descriptor *dd_found = NULL;

	void aux(struct dev_descriptor *dd, void *ptr) {
		if(strcmp(dd->name, name) == 0) {
			dd_found = dd;
		}
	}

	dev_foreach(type, aux, NULL);

	return dd_found;
}


static rv on_cmd_dev(uint8_t argc, char **argv)
{
	void aux(struct dev_descriptor *dd, void *ptr)
	{
		printd("0x%x: %d - %s\n", dd->dev, dd->type, dd->name);
	}

	dev_foreach(DEV_TYPE_ANY, aux, NULL);

	return RV_OK;
}

CMD_REGISTER(dev, on_cmd_dev, "l[ist]");

/*
 * End
 */

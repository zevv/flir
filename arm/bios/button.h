#ifndef bios_button_h
#define bios_button_h

enum button_state {
	BUTTON_STATE_RELEASE,
	BUTTON_STATE_PUSH,
};

struct dev_button;

rv button_init(struct dev_button *dev);

#endif


#ifndef bios_led_h
#define bios_led_h

enum led_state_t {
	LED_STATE_OFF,
	LED_STATE_ON,
	LED_STATE_BLINK,
};

struct dev_led;

rv led_init(struct dev_led *dev);
rv led_set(struct dev_led *dev, enum led_state_t state);

#endif

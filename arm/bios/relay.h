#ifndef bios_relay_h
#define bios_relay_h

enum relay_state_t {
	RELAY_STATE_OPEN,
	RELAY_STATE_CLOSED,
};

struct dev_relay;

rv relay_init(struct dev_relay *dev);
rv relay_set(struct dev_relay *relay, enum relay_state_t state);
rv relay_get(struct dev_relay *relay, enum relay_state_t *state);

#endif

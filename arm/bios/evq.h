
/*
 * Event queue handling
 *
 * When adding an event type make sure to:
 *
 * - add the EV_XXXX definition
 * - define a matching ev_xxxx struct for the event data
 * - add the ev_xxxx struct to the event_t union
 * - add the new event to event_size() in evq.c
 */


#ifndef bios_event_h
#define bios_event_h

#include <stdint.h>
#include "bios/button.h"

enum {
	EV_BOOT,
	EV_TICK_1HZ,
	EV_TICK_10HZ,
	EV_TICK_100HZ,
	EV_UART,
	EV_CAN,
	EV_BUTTON,
};

typedef uint8_t evtype_t;

/*
 * Data for the various event types
 */

struct ev_boot {
	evtype_t type;
} __attribute__ (( __packed__ ));

struct ev_tick_1hz {
	evtype_t type;
} __attribute__ (( __packed__ ));

struct ev_tick_10hz {
	evtype_t type;
} __attribute__ (( __packed__ ));

struct ev_tick_100hz {
	evtype_t type;
} __attribute__ (( __packed__ ));

struct ev_can {
	evtype_t type;
	struct dev_can *dev;
	uint32_t id: 29;
	unsigned extended: 1;
	uint8_t len;
	uint8_t data[8];
} __attribute__ (( __packed__ ));

struct ev_button {
	evtype_t type;
	struct dev_button *dev;
	enum button_state state;
} __attribute__ (( __packed__ ));

struct ev_uart {
	evtype_t type;
	uint8_t data;
} __attribute__ (( __packed__ ));

struct ev_raw {
	evtype_t type;
	uint8_t data[1];
} __attribute__ (( __packed__ ));

/*
 * The main event type, union of all possible types.
 */

typedef union {
	evtype_t type;
	struct ev_boot boot;
	struct ev_tick_1hz tick_1hz;
	struct ev_tick_10hz tick_10hz;
	struct ev_tick_100hz tick_100hz;
	struct ev_can can;
	struct ev_button button;
	struct ev_uart uart;
	struct ev_raw raw;
} event_t;


/*
 * Event handlers
 */

typedef void (*evq_handler)(event_t *ev, void *data);

struct evq_handler_t {
	evtype_t type;
	evq_handler fn;
	void *fndata;
};


/*
 * An instance of this structure is created in a special ELF section. At
 * runtime, the special section is treated as an array of these.
 */

/*lint -e9024 -e528 */
#define PASTE1(x, y) x ## y
#define PASTE2(x, y) PASTE1(x, y)
#define EVH_NAME PASTE2(evh, __LINE__)

#define EVQ_REGISTER(t, f) \
	static const struct evq_handler_t                    \
	__attribute__ (( __used__ ))                         \
	__attribute((aligned(4)))                            \
	__attribute__ (( __section__ ( "evhandler" )))       \
	EVH_NAME = {                                         \
		.type = t,                                   \
		.fn = f                                      \
	}

/*
 * Event queue API
 */

void evq_push(event_t *ev);
void evq_wait(event_t *ev);
void evq_pop(event_t *ev);
void evq_register(evtype_t type, evq_handler fn, void *fndata);
uint8_t evq_get_load(void);

#endif


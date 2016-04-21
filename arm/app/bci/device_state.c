
#include "bios/bios.h"
#include "bios/evq.h"
#include "bios/printd.h"
#include "bios/relay.h"
#include "bios/adc.h"
#include "bios/log.h"

#include "app/bci/device_state.h"

#if 0
#define debug printd
#else
#define debug(...)
#endif

void state_INIT_to(void);
void state_INIT_do(enum device_cmd cmd);
void state_OFF_to(void);
void state_OFF_do(enum device_cmd cmd);
void state_ON1_to(void);
void state_ON1_do(enum device_cmd cmd);
void state_ON2_to(void);
void state_ON2_do(enum device_cmd cmd);
void state_ON_to(void);
void state_ON_do(enum device_cmd cmd);
void state_PRE1_to(void);
void state_PRE1_do(enum device_cmd cmd);
void state_PRE_to(void);
void state_PRE_do(enum device_cmd cmd);


struct state_handler {
	const char *name;
	void (*fn_to)(void);
	void (*fn_do)(enum device_cmd cmd);
};


static const struct state_handler state_handler_table[] = {
	{ "init", state_INIT_to,  state_INIT_do },
	{ "off",  state_OFF_to,   state_OFF_do  },
	{ "on1",  state_ON1_to,   state_ON1_do  },
	{ "on2",  state_ON2_to,   state_ON2_do  },
	{ "on",   state_ON_to,    state_ON_do   },
	{ "pre1", state_PRE1_to, state_PRE1_do  },
	{ "pre",  state_PRE_to,   state_PRE_do  },
};

static uint8_t ticks = 0;
static enum device_state state = DEVICE_STATE_INIT;


static void set_state(enum device_state next, const char *reason)
{
	logd(LG_INF, "state %s->%s: %s",
			state_handler_table[state].name,
			state_handler_table[next].name,
			reason);
	state = next;
	state_handler_table[state].fn_to();
}


/*
 * INIT STATE
 */

void state_INIT_to(void)
{
}

void state_INIT_do(enum device_cmd cmd)
{
	debug("state_INIT_do\n");
	set_state(DEVICE_STATE_OFF, "init");
}


/*
 * OFF STATE
 */

/*Static state*/

void state_OFF_to(void)
{
	debug("state_OFF_to\n");
	(void)relay_set(&relay_main_high, RELAY_STATE_OPEN);
	(void)relay_set(&relay_main_low, RELAY_STATE_OPEN);
	(void)relay_set(&relay_precharge, RELAY_STATE_OPEN);
	(void)relay_set(&relay_reset, RELAY_STATE_OPEN);
}


void state_OFF_do(enum device_cmd cmd)
{
	if(cmd == DEVICE_CMD_ON) {
		set_state(DEVICE_STATE_ON1, "on");
	}

	if(cmd == DEVICE_CMD_PRECHARGE) {
		set_state(DEVICE_STATE_PRE1, "precharge");
	}

	if(cmd == DEVICE_CMD_RESET) {
		set_state(DEVICE_STATE_OFF, "reset");
	}
}


/*
 * ON1 STATE
 */

void state_ON1_to(void)
{
	(void)relay_set(&relay_main_high, RELAY_STATE_OPEN);
	(void)relay_set(&relay_main_low, RELAY_STATE_CLOSED);
	(void)relay_set(&relay_precharge, RELAY_STATE_CLOSED);
	(void)relay_set(&relay_reset, RELAY_STATE_CLOSED);
	ticks = 0;
}

void state_ON1_do(enum device_cmd cmd)
{
	int16_t voltage_source;
	int16_t voltage_CAN = 13000;
	int16_t difference;
	int16_t delta = 1000;

	if(cmd == DEVICE_CMD_TICK_10HZ) {

		ticks ++;

		if(ticks == 30u) {

			voltage_source = adc_continuous_read (&supply_voltage, SUPPLY_VOLTAGE);
			debug("voltage_source:%ld\n", voltage_source);

			if(voltage_source >= voltage_CAN) {
				debug("voltage_source >= voltage_CAN\n");
				set_state(DEVICE_STATE_ON2, "ON1 30 ticks");
			} else {
				difference = voltage_CAN - voltage_source;
				if (difference < delta){
					set_state(DEVICE_STATE_ON2, "ON1 30 ticks");
				} else{
					set_state(DEVICE_STATE_ON1, "ON1 (voltage_CAN - voltage_source) > 1000mV");
				}
			}
		}
	}

	if(cmd == DEVICE_CMD_OFF) {
		set_state(DEVICE_STATE_OFF, "off");
	}

	if(cmd == DEVICE_CMD_PRECHARGE) {
		set_state(DEVICE_STATE_PRE1, "precharge");
	}

	if(cmd == DEVICE_CMD_RESET) {
		set_state(DEVICE_STATE_OFF, "reset");
	}
}


/*
 * ON2 STATE
 */

void state_ON2_to(void)
{
	(void)relay_set(&relay_main_high, RELAY_STATE_CLOSED);
	(void)relay_set(&relay_main_low, RELAY_STATE_CLOSED);
	(void)relay_set(&relay_precharge, RELAY_STATE_CLOSED);
	(void)relay_set(&relay_reset, RELAY_STATE_CLOSED);
	ticks = 0;
}

void state_ON2_do(enum device_cmd cmd)
{
	if(cmd == DEVICE_CMD_TICK_10HZ) {
		ticks ++;
		if(ticks == 10u){
			set_state(DEVICE_STATE_ON, "ON2 10 ticks");
		}
	}
	
	if(cmd == DEVICE_CMD_OFF) {
		set_state(DEVICE_STATE_OFF, "off");
	}

	if(cmd == DEVICE_CMD_PRECHARGE) {
		set_state(DEVICE_STATE_PRE1, "precharge");
	}

	if(cmd == DEVICE_CMD_RESET) {
		set_state(DEVICE_STATE_OFF, "reset");
	}
}


/*
 * ON STATE
 */

/*Static state*/

void state_ON_to(void)
{
	(void)relay_set(&relay_main_high, RELAY_STATE_CLOSED);
	(void)relay_set(&relay_main_low, RELAY_STATE_CLOSED);
	(void)relay_set(&relay_precharge, RELAY_STATE_OPEN);
	(void)relay_set(&relay_reset, RELAY_STATE_OPEN);
}

void state_ON_do(enum device_cmd cmd)
{
	if(cmd == DEVICE_CMD_OFF) {
		set_state(DEVICE_STATE_OFF, "off");
	}

	if(cmd == DEVICE_CMD_PRECHARGE) {
		set_state(DEVICE_STATE_PRE1, "precharge");
	}

	if(cmd == DEVICE_CMD_RESET) {
		set_state(DEVICE_STATE_OFF, "reset");
	}
}


/*
 * PRE1 STATE
 */

/*Temporary state*/

void state_PRE1_to(void)
{
	(void)relay_set(&relay_main_low, RELAY_STATE_CLOSED);
	(void)relay_set(&relay_precharge, RELAY_STATE_CLOSED);
	(void)relay_set(&relay_reset, RELAY_STATE_CLOSED);
	ticks = 0;
}

void state_PRE1_do(enum device_cmd cmd)
{
	if(cmd == DEVICE_CMD_TICK_10HZ) {
		ticks ++;
		if(ticks == 10u) {
			set_state(DEVICE_STATE_PRE, "PRE1 10 ticks");
		}
	}

	if(cmd == DEVICE_CMD_OFF) {
		set_state(DEVICE_STATE_OFF, "off");
	}

	if(cmd == DEVICE_CMD_ON) {
		set_state(DEVICE_STATE_ON1, "on");
	}

	if(cmd == DEVICE_CMD_RESET) {
		set_state(DEVICE_STATE_OFF, "reset");
	}
}


/*
 * PRE STATE
 */

/*Static state*/

void state_PRE_to(void)
{
	(void)relay_set(&relay_main_high, RELAY_STATE_OPEN);
	(void)relay_set(&relay_main_low, RELAY_STATE_CLOSED);
	(void)relay_set(&relay_precharge, RELAY_STATE_CLOSED);
	(void)relay_set(&relay_reset, RELAY_STATE_CLOSED);
}

void state_PRE_do(enum device_cmd cmd)
{
	if(cmd == DEVICE_CMD_OFF) {
		set_state(DEVICE_STATE_OFF, "off");
	}

	if(cmd == DEVICE_CMD_ON) {
		set_state(DEVICE_STATE_ON1, "on");
	}

	if(cmd == DEVICE_CMD_RESET) {
		set_state(DEVICE_STATE_OFF, "reset");
	}
}


/*
 * Public functions
 */

rv device_state_get(enum device_state *state_out)
{
	*state_out = state;
	return RV_OK;
}


rv device_state_cmd(enum device_cmd cmd)
{
	state_handler_table[state].fn_do(cmd);
	return RV_OK;
}


/*
 * Event handlers
 */

static void on_ev_tick(event_t *ev, void *data)
{
	device_state_cmd(DEVICE_CMD_TICK_10HZ);
}

EVQ_REGISTER(EV_TICK_10HZ, on_ev_tick);



/*
 * End
 */

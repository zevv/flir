
#include <string.h>

#include "bios/cmd.h"
#include "bios/evq.h"
#include "bios/printd.h"
#include "bios/uart.h"

#include "lib/config.h"

#include "app/bci/modules.h"

static uint32_t enable_mask = 0;

/*
 * Iterator for all modules linked in the code
 */

static void foreach_mod(void (*fn)(const struct module *mod, void *data), void *data)
{
	/*lint -esym(526, __start_mod) */
	/*lint -esym(526, __stop_mod) */

	extern struct module __start_mod;
	extern struct module __stop_mod;

	const struct module *mod;
	mod = &__start_mod;
	for(;;) {
		if(mod >= &__stop_mod) {
			break;
		}
		fn(mod, data);
		mod++;
	}
}


/*
 * Find given module by name
 */

static const struct module *module_find_by_name(const char *name)
{
	const struct module *mod_found = NULL;

	void aux(const struct module *mod, void *data)
	{
		if(strcmp(mod->name, name) == 0) {
			mod_found = mod;
		}
	}

	foreach_mod(aux, NULL);
	return mod_found;
}


/*
 * Find given module by id
 */

static const struct module *module_find_by_id(uint8_t id)
{
	const struct module *mod_found = NULL;

	void aux(const struct module *mod, void *data)
	{
		if(mod->id == id) {
			mod_found = mod;
		}
	}

	foreach_mod(aux, NULL);
	return mod_found;
}



/*
 * Start a module
 */

static rv module_start(const struct module *mod)
{
	rv r = RV_OK;

	if(!(mod->state->flags & MODULE_STATE_FLAG_ENABLED)) {

		printd("Starting module '%s'\n", mod->name);

		if(mod->fn_init) {
			r = mod->fn_init(mod);
		}
		if(r == RV_OK) {
			mod->state->flags |= MODULE_STATE_FLAG_ENABLED;
		}
	} else {
		r = RV_EBUSY;
	}

	return r;
}


/*
 * Stop a module
 */

static rv module_stop(const struct module *mod)
{
	rv r = RV_OK;

	if(mod->state->flags & MODULE_STATE_FLAG_ENABLED) {
		if(mod->fn_exit) {
			r = mod->fn_exit(mod);
		}
		if(r == RV_OK) {
			mod->state->flags &= ~MODULE_STATE_FLAG_ENABLED;
		}
	} else {
		r = RV_EBUSY;
	}

	return r;
}


rv module_printd(const struct module *mod, const char *fmt, ...)
{
	va_list va;
	
	if(mod->state->flags & MODULE_STATE_FLAG_DEBUGGING) {

		va_start(va, fmt);
		vfprintd(uart_tx, fmt, va);
		va_end(va);
		uart_tx('\n');
	}

	return RV_OK;
}



/*
 * Call timer tick function for all enabled modules
 */



static void on_ev_tick_10hz(event_t *ev, void *data)
{
	uint32_t mask = 0;

	void aux(const struct module *mod, void *data)
	{
		if(mod->state->flags & MODULE_STATE_FLAG_ENABLED) {
			mask |= (1<<mod->id);
			if(mod->fn_timer) {
				mod->fn_timer(mod);
			}
		}
	}

	foreach_mod(aux, NULL);

	if(mask != enable_mask) {
		enable_mask = mask;
		config_write("modules", &enable_mask, sizeof(enable_mask));
	}
}

EVQ_REGISTER(EV_TICK_10HZ, on_ev_tick_10hz);


/*
 * Pass received CAN frames to all registered modules
 */

static void module_recv_can(const struct module *mod, void *data)
{
	event_t *ev = data;
	
	if(mod->state->flags & MODULE_STATE_FLAG_ENABLED) {
		if(mod->fn_recv) {
			/* todo: add filtering */
			mod->fn_recv(mod, 
				ev->can.extended ? CAN_ADDRESS_MODE_EXT : CAN_ADDRESS_MODE_STD,
				ev->can.id, ev->can.data, ev->can.len);
		}
	}
}


static void on_ev_can(event_t *ev, void *data)
{
	foreach_mod(module_recv_can, ev);
}
	
EVQ_REGISTER(EV_CAN, on_ev_can);




/*
 * Start all enabled modules
 */



rv modules_start(void)
{
	rv r = RV_OK;

	void aux(const struct module *mod, void *data)
	{
		if(enable_mask & (1 << mod->id)) {
			module_start(mod);
		}
	}

	r = config_read("modules", &enable_mask, sizeof(enable_mask));
	foreach_mod(aux, NULL);

	return r;
}


/*
 * Get info for module by ID
 */


rv module_get_info(uint8_t id, const struct module **module)
{
	*module = module_find_by_id(id);
	return (*module != NULL) ? RV_OK : RV_ENODEV;
}


/*
 * Cmd line handler
 */

static rv on_cmd_module(uint8_t argc, char **argv)
{
	rv r = RV_EINVAL;
			
	const struct module *mod;

	if(argc >= 1u) {
		char cmd = argv[0][0];

		if(argc >= 2) {
			mod = module_find_by_name(argv[1]);
			if(mod) {
				if(cmd == 's') {
					r = module_start(mod);
				}
				if(cmd == 'p') {
					r = module_stop(mod);
				}
				if(cmd == 'd') {
					mod->state->flags ^= MODULE_STATE_FLAG_DEBUGGING;
					r = RV_OK;
				}
			} else {
				r = RV_ENODEV;
			}
		}

		if(cmd == 'l') {

			void aux(const struct module *mod, void *data)
			{
				printd("%d: %c%c: %s / %s\n", 
						mod->id,
						(mod->state->flags & MODULE_STATE_FLAG_ENABLED) ? 'e' : '-', 
						(mod->state->flags & MODULE_STATE_FLAG_DEBUGGING) ? 'd' : '-', 
						mod->name, mod->description);
			}

			foreach_mod(aux, NULL);
			r = RV_OK;
		}
	}

	return r;
}

CMD_REGISTER(module, on_cmd_module, "l[ist] | s[tart] <id> | [sto]p id");

/*
 * End
 */

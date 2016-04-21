#ifndef app_bci_modules_modules_h
#define app_bci_modules_modules_h

#include <stdlib.h>
#include "bios/can.h"
#include "bios/bios.h"


typedef enum {
	MOD_OP_NOOPTIONS  = 0x00,
	MOD_OP_ALLWAYSEN  = 0x01,
	MOD_OP_INITIALEN  = 0x02,
	MOD_OP_HIDE       = 0x04,
	MOD_OP_USERSELECT = 0x08,
} module_options;

struct module_state {
	uint8_t flags;
};

struct api_can_addr {
	uint32_t addr;
	uint32_t mask;
};


#define MODULE_STATE_FLAG_ENABLED (1<<0)
#define MODULE_STATE_FLAG_DEBUGGING (1<<1)

struct module {
	uint8_t id;
	struct module_state *state;
	const char *name;
	const char *description;
	uint32_t revision;
	module_options options;
	struct api_can_addr *can_filter;
	rv (*fn_init)(const struct module *mod);
	rv (*fn_idle)(const struct module *mod);
	rv (*fn_recv)(const struct module *mod, enum can_address_mode_t fmt, uint32_t can_id, void *data, size_t len);
	rv (*fn_timer)(const struct module *mod);
	rv (*fn_exit)(const struct module *mod);
};


#define MOD_REGISTER(...)                         \
	static struct module_state mod_state; \
	static const struct module            \
	__attribute((__used__))                   \
	__attribute((aligned(4)))                 \
	__attribute__ (( __section__ ( "mod" )))  \
	mod_def = {                               \
		.state = &mod_state,              \
		__VA_ARGS__                       \
	};


rv modules_start(void);
rv module_get_info(uint8_t id, const struct module **module);
rv module_printd(const struct module *mod, const char *fmt, ...);

#endif

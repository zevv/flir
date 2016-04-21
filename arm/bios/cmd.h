#ifndef bios_cmd_h
#define bios_cmd_h

#include <stdint.h>

#include "bios/bios.h"

typedef rv (*cmd_handler)(uint8_t argc, char **argv);

struct cmd_handler_t {
	char const *name;
	cmd_handler fn;
	const char *help;
};

#define CMD_REGISTER(n, f, h)                     \
	static const struct cmd_handler_t         \
	__attribute((__used__))                   \
	__attribute((aligned(4)))                 \
	__attribute__ (( __section__ ( "cmd" )))  \
	cmd_handler_ ## n   = {                   \
		.name = #n,                       \
		.fn = f,                          \
		.help = h                         \
	}

#endif

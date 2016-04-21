
#include <string.h>

#include "bios/bios.h"
#include "bios/uart.h"
#include "bios/cmd.h"
#include "bios/log.h"
#include "bios/printd.h"
#include "bios/arch.h"

static const char *level_str[] = {
	[LG_FTL] = "ftl",
	[LG_WRN] = "wrn",
	[LG_TST] = "tst",
	[LG_INF] = "inf",
	[LG_DBG] = "dbg",
	[LG_DMP] = "dmp",
};

struct log_rb {
	uint16_t head;
	uint16_t tail;
	char buf[BIOS_LOG_RB_SIZE];
};

static struct log_rb log;


static rv to_log(uint8_t c)
{
	log.buf[log.head] = c;
	log.head = (log.head + 1) % sizeof(log.buf);
	if(log.head == log.tail) {
		log.tail = (log.tail + 1) % sizeof(log.buf);
	}
	return RV_OK;
}


void _logd(enum log_level level, const char *file, const char *fmt, ...)
{
	uint32_t t = arch_get_ticks() / 1000;

	const char *f = strrchr(file, '/');
	f = f ? f+1 : file;

	uint16_t i = log.head;

	fprintd(to_log, "%02d:%02d:%02d|%s|%-8.8s|", (t/( 60*60))%24, (t/60)%60, t%60, level_str[level], f);
	va_list va;
	va_start(va, fmt);
	vfprintd(to_log, fmt, va);
	va_end(va);
	to_log('\n');

	uart_tx('\r');
	while(i != log.head) {
		uart_tx(log.buf[i]);
		i = (i + 1) % sizeof(log.buf);
	}
	uart_tx('\e');
	uart_tx('[');
	uart_tx('K');
}


static rv on_cmd_log(uint8_t argc, char **argv)
{
	rv r = RV_EINVAL;

	if(argc >= 1) {
		char cmd = argv[0][0];

		if(cmd == 'c') {
			log.head = log.tail = 0;
			r = RV_OK;
		}
	} else {
		uint16_t i = log.tail;
		while(i != log.head) {
			uart_tx(log.buf[i]);
			i = (i + 1) % sizeof(log.buf);
		}
		r = RV_OK;
	}

	return r;
}


CMD_REGISTER(log, on_cmd_log, "c[lear]");

/*
 * End
 */


#include <stdint.h>
#include <stddef.h>

#include "bios/bios.h"
#include "bios/eeprom.h"
#include "bios/log.h"
#include "bios/printd.h"


#define LOGBUF_SIZE 32u

static uint8_t logbuf[LOGBUF_SIZE];
static size_t loglen = 0;

static size_t rb_head = 0;
static const size_t rb_size = 4 * 1024;


static void add_u8(uint8_t v)
{
	if((LOGBUF_SIZE - loglen) > sizeof(uint8_t)) {
		logbuf[loglen] = v;
		loglen ++;
	}
}


static void add_u32(uint32_t v)
{
	if((LOGBUF_SIZE - loglen) > sizeof(uint32_t)) {
		logbuf[loglen] = (uint8_t)((v >> 24) & 0xffu);
		loglen ++;
		logbuf[loglen] = (uint8_t)((v >> 16) & 0xffu);
		loglen ++;
		logbuf[loglen] = (uint8_t)((v >>  8) & 0xffu);
		loglen ++;
		logbuf[loglen] = (uint8_t)((v >>  0) & 0xffu);
		loglen ++;
	}
}

static void add_s32(int32_t v)
{
	uint32_t vu = (uint32_t)v;
	add_u32(vu);
}


void log_init(void)
{
}


void log_begin(enum event_type ev)
{
	loglen = 0;
	add_u8((uint8_t )ev);
	add_u32(0x11ee3344);
}


void log_U(voltage U)
{
	add_s32(U);
}


void log_commit(void)
{
	size_t todo = loglen;
	size_t done = 0;
		
	print_hex(logbuf, loglen);

	while(todo > 0u) {

		size_t len = todo;
		size_t avail = rb_size - rb_head;

		if(len > avail) {
			len = avail;
		}

		eeprom_write(&DEFAULT_EEPROM_DEV, rb_head, &logbuf[done], len);

		todo -= len;
		done += len;

		rb_head += len;
		if(rb_head >= rb_size) {
			rb_head -= rb_size;
		}
	}
	
}



/*
 * End
 */



#include <stddef.h>
#include <assert.h>

#include "bios/bios.h"
#include "bios/evq.h"
#include "bios/printd.h"
#include "bios/cmd.h"
#include "bios/arch.h"

#define EVQ_SIZE 256u
#define EVQ_MAX_REGISTRATIONS 32u

typedef size_t evq_index;


struct evq {
	volatile evq_index head;
	volatile evq_index tail;
	volatile uint8_t data[EVQ_SIZE];
	uint32_t xruns;
	evq_index used_max;

	struct evq_handler_t reg_list[EVQ_MAX_REGISTRATIONS];
	uint8_t reg_count;

	uint32_t ticks_busy;
	uint32_t load;
};

static struct evq evq;


static size_t ev_size(evtype_t type)
{
	size_t s = 0;

	switch(type) {
		case EV_TICK_1HZ: 
			s = sizeof(struct ev_tick_1hz); 
			break;
		case EV_TICK_10HZ: 
			s = sizeof(struct ev_tick_10hz); 
			break;
		case EV_TICK_100HZ: 
			s = sizeof(struct ev_tick_100hz); 
			break;
		case EV_UART: 
			s = sizeof(struct ev_uart); 
			break;
		case EV_CAN: 
			s = sizeof(struct ev_can); 
			break;
		case EV_BUTTON: 
			s = sizeof(struct ev_button); 
			break;
		case EV_BOOT: 
			s = sizeof(struct ev_boot); 
			break;
		default: 
			/* nothing */
			break;
	}

	if(s == 0u) {
		printd("unknown ev %d\n", type);
		assert(s != 0u);
	}

	return s;
}


static evq_index evq_used(void)
{
	evq_index head = evq.head;
	evq_index tail = evq.tail;
	evq_index used = (head >= tail) ? (head - tail) : (EVQ_SIZE + head - tail);
	if(used > evq.used_max) {
		evq.used_max = used;
	}
	return used;
}


static evq_index evq_room(void)
{
	evq_index used = evq_used();
	evq_index room = (EVQ_SIZE - used) - 1u;
	return room;
}


static void pushb(uint8_t b)
{
	evq.data[evq.head] = b;
	evq.head++;
	if(evq.head == EVQ_SIZE) {
		evq.head = 0;
	}
}


static uint8_t popb(void)
{
	uint8_t b = evq.data[evq.tail];
	evq.tail++;
	if(evq.tail == EVQ_SIZE) {
		evq.tail = 0;
	}
	return b;
}


void evq_push(event_t *ev)
{
	evq_index room = evq_room();
	evq_index size = ev_size(ev->type);

	arch_irq_disable();

	if(room >= size) {
		pushb((uint8_t)ev->type);
		const uint8_t *p = ev->raw.data; 

		evq_index i;
		for(i=0; i<(size - 1u); i++) {
			pushb(*p);
			p++;
		}
	} else {
		evq.xruns ++;
	}

	arch_irq_enable();
}


void evq_pop(event_t *ev)
{
	arch_irq_disable();

	evq_index head = evq.head;
	evq_index tail = evq.tail;

	if(head != tail) {
		/*lint -e9030 -e9034 */
		ev->type = (evtype_t) popb();
		size_t size = ev_size(ev->type);
		size_t i;
		for(i=0; i<(size - 1u); i++) {
			ev->raw.data[i] = popb();
		}
	}

	arch_irq_enable();
}


static void evq_handler_foreach(void (*cb)(struct evq_handler_t *h))
{
	/* compiled in handlers */

#ifndef _lint
	extern struct evq_handler_t __start_evhandler;
	extern struct evq_handler_t __stop_evhandler;
	struct evq_handler_t *h = &__start_evhandler;

	while(h < &__stop_evhandler) {
		cb(h);
		h++;
	}
#endif

	/* dynamic registered handlers */

	uint8_t i;
	for(i=0; i<evq.reg_count; i++) {
		struct evq_handler_t *h = &evq.reg_list[i];
		cb(h);
	}
}


void evq_wait(event_t *ev)
{

	for(;;) {
		evq_index head = evq.head;
		evq_index tail = evq.tail;
		if(head == tail) {
			arch_idle();
		} else {
			break;
		}
	}

	evq_pop(ev);

	void aux(struct evq_handler_t *h)
	{
		if(h->type == ev->type) {
			h->fn(ev, h->fndata);
		}
	}

	uint32_t t1 = arch_get_ticks();

	evq_handler_foreach(aux);
	
	uint32_t t2 = arch_get_ticks();
	evq.ticks_busy += t2 - t1;
}


void evq_register(evtype_t type, evq_handler fn, void *fndata)
{
	assert(evq.reg_count < EVQ_MAX_REGISTRATIONS);

	struct evq_handler_t *reg = &evq.reg_list[evq.reg_count];
	reg->type = type;
	reg->fn = fn;
	reg->fndata = fndata;
	evq.reg_count ++;
}


uint8_t evq_get_load(void)
{
	return evq.load;
}


static void on_ev_tick_1hz(event_t *ev, void *data)
{
	uint32_t busy = evq.ticks_busy / 10;
	evq.load = (evq.load + busy) / 2;
	evq.ticks_busy = 0;
}

EVQ_REGISTER(EV_TICK_1HZ, on_ev_tick_1hz);


static rv on_cmd_evq(uint8_t argc, char **argv)
{
	rv r = RV_EINVAL;

	if(argc >= 1u) {
		char cmd = argv[0][0];
		
		if(cmd == 's') {
			printd("load:%d ", evq_get_load());
			printd("xruns:%d ", evq.xruns);
			printd("size:%d/%d/%d\n", evq_used(), evq.used_max, EVQ_SIZE);
			r = RV_OK;
		}

		if(cmd == 'l') {
			void aux(struct evq_handler_t *h)
			{
				printd("0x%08x: %d %p\n", h->fn, h->type, h->fndata);
			}
			evq_handler_foreach(aux);
			r = RV_OK;
		}
	}

	return r;
}

CMD_REGISTER(evq, on_cmd_evq, "l[ist] | s[tats]");


/*
 * End
 */



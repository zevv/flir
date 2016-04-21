
#include <winsock2.h>
#include <stdint.h>

#include "bios/bios.h"
#include "bios/arch.h"
#include "bios/evq.h"
#include "bios/printd.h"

#include "arch/win32/mainloop.h"

static uint32_t jiffies;

uint32_t arch_get_ticks(void)
{
	return jiffies;
}


int on_timer(void *t)
{
	static uint8_t t1 = 0;
	static uint8_t t2 = 0;
	static uint8_t t3 = 0;
        event_t ev;

	jiffies ++;

	t1 ++;
	if(t1 == 10) {

		t1 = 0;
		ev.type = EV_TICK_100HZ;
		evq_push(&ev);

		t2 ++;
		if(t2 == 10) {
			t2 = 0;
			ev.type = EV_TICK_10HZ;
			evq_push(&ev);
		
			t3++;
			if(t3 == 10) {
				t3 = 0;
				ev.type = EV_TICK_1HZ;
				evq_push(&ev);
			}
		}
	}

	return 1;
}


void arch_init(void)
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	mainloop_timer_add(1, on_timer, NULL);
}


void arch_idle(void)
{
	mainloop_poll();
}


void arch_reboot(void)
{
	printd("reboot\n");
}


uint32_t arch_get_unique_id(void)
{
	return gethostid();
}


void arch_irq_disable(void)
{
}


void arch_irq_enable(void)
{
}


/*
 * End
 */

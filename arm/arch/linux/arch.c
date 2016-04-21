
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <errno.h>

#include <SDL/SDL.h>

#include "bios/bios.h"
#include "bios/dev.h"
#include "bios/evq.h"
#include "bios/printd.h"
#include "bios/arch.h"
#include "bios/can.h"
#include "bios/relay.h"

#include "arch/linux/mainloop.h"

SDL_Surface *screen = NULL;

rv errno_to_rv(int e)
{
	rv r = RV_EIO;

	if(e == EIO) r = RV_EIO;
	if(e == EAGAIN) r = RV_EBUSY;

	return r;
}

	
static int on_tick_1hz(void *data)
{
	event_t e;
	e.type = EV_TICK_1HZ;
	evq_push(&e);
	return 1;
}


static void draw_relay(int x, int y)
{
	void aux(struct dev_descriptor *dd, void *ptr)
	{
		struct dev_relay *dev = dd->dev;
		enum relay_state_t st;
		relay_get(dev, &st);

		SDL_Rect r = { x, y, 8, 8 };
		uint32_t color = (st == RELAY_STATE_OPEN) ? 0x00202020 : 0x00c0c0c0;
		SDL_FillRect(screen, &r, color);

		x += 10;
	}

	dev_foreach(DEV_TYPE_RELAY, aux, NULL);
}


static void graph(int x, int y, int w, int h, int val, uint32_t color)
{
	/* Scroll */

	SDL_Rect r1 = { x-1, y, screen->w-20, h };
	SDL_Rect r2 = {   x, y, screen->w-20, h };
	SDL_BlitSurface(screen, &r1, screen, &r2);

	/* Background */

	SDL_Rect r3 = {  x, y, 1, h };
	SDL_FillRect(screen, &r3, 0x303030);

	/* Graph */

	SDL_Rect r4 = {  x, y+h-val, 1, val };
	SDL_FillRect(screen, &r4, color);
}


static void draw_load_graph(int x, int y)
{
	int n = evq_get_load() * 0.25 + 1;
	graph(x, y, screen->w-20, 25, n, 0xff0000);
}


static void draw_can_graph(int x, int y)
{
	static uint32_t prev[32][2];
	static uint32_t color[2] = { 0xffcc00, 0x00ccff };
	int i = 0;
	int h = 15;

	void aux(struct dev_descriptor *dd, void *ptr)
	{
		struct dev_can *dev = dd->dev;
		struct can_stats st;
		
		can_get_stats(dev, &st);

		int j;
		for(j=0; j<2; j++) {

			double val = (j == 0) ? st.tx_total : st.rx_total;
			double n = log(val - prev[i][j] + 1) * 5 + 1;
			prev[i][j] = val;

			graph(x, y, screen->w-20, h, n, color[j]);

			y += h + 3;
		}
		
		i++;

		y += 3;
	}
	
	dev_foreach(DEV_TYPE_CAN, aux, NULL);
}


static int on_tick_10hz(void *data)
{
	event_t e;
	e.type = EV_TICK_10HZ;
	evq_push(&e);
	return 1;
}


static int on_tick_100hz(void *data)
{
	static int n = 0;

	if((n%4) == 0) {
		draw_relay(80, 10);
		draw_can_graph(10, 70);
	}
	if((n%10) == 0) {
		draw_load_graph(10, 30);
	}
		
	SDL_Flip(screen);
	n++;

	event_t e;
	e.type = EV_TICK_100HZ;
	evq_push(&e);
	return 1;
}


static void bye(void)
{
	SDL_Quit();
	printf("\n");
}


void arch_init(void)
{
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE);
	SDL_WM_SetCaption("superb", NULL);
	screen = SDL_SetVideoMode(256, 168, 32, SDL_RESIZABLE);
	screen = SDL_SetVideoMode(256, 168, 32, SDL_RESIZABLE);

	atexit(bye);

	mainloop_timer_add(1,   0, on_tick_1hz,   NULL);
	mainloop_timer_add(0, 100, on_tick_10hz,  NULL);
	mainloop_timer_add(0,  10, on_tick_100hz, NULL);
}


void arch_idle(void)
{
	mainloop_poll();

	SDL_Event ev;

	while(SDL_PollEvent(&ev)) {

		if(ev.type == SDL_VIDEORESIZE) {
			screen = SDL_SetVideoMode(ev.resize.w, ev.resize.h, 32, SDL_RESIZABLE);

		}
	}
}


void arch_reboot(void)
{
	exit(0);
}


uint32_t arch_get_ticks(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	uint32_t ticks = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
	return ticks;
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

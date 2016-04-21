
/*
 * Win32 implementation of mainloop. Quite inefficient with lots of O(n)
 * behaviour, but will do for now.
 */

#include <stdint.h>
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>


#include "mainloop.h"

#define LIST_ADD_ITEM(list, item) \
	item->prev = NULL; \
	item->next = list; \
	if(item->next) item->next->prev = item; \
	list = item;

#define LIST_REMOVE_ITEM(list, item) \
	if(item == list) list=item->next; \
	if(item->prev) item->prev->next = item->next; \
	if(item->next) item->next->prev = item->prev;

#define LIST_FOREACH(list, cursor, cursor_next) \
	for(cursor=list; cursor_next = (cursor) ? cursor->next : NULL, cursor; cursor=cursor_next) 


struct mainloop_handle_t {
	HANDLE h;
	enum handle_type type;
	int (*handler)(HANDLE h, void *user);
	void *user;
	int remove;
	struct mainloop_handle_t *prev;
	struct mainloop_handle_t *next;
};

struct mainloop_timer_t {
	uint64_t when;
	uint64_t interval;
	int (*handler)(void *user);
	void *user;
	int remove;
	struct mainloop_timer_t *prev;
	struct mainloop_timer_t *next;
};

struct mainloop_signal_t {
	int signum;
	int (*handler)(int signum, void *user);
	void *user;
	int signaled;
	int remove;
	struct mainloop_signal_t *prev;
	struct mainloop_signal_t *next;
};


struct mainloop_handle_t *mainloop_handle_list = NULL;
struct mainloop_timer_t *mainloop_timer_list = NULL;
struct mainloop_signal_t *mainloop_signal_list = NULL;
int mainloop_running = 1;

static uint64_t get_time(void)
{
	SYSTEMTIME  system_time;
	FILETIME    file_time;
	uint64_t    time;

	GetSystemTime(&system_time);
	SystemTimeToFileTime(&system_time, &file_time);

	time =  ((uint64_t)file_time.dwLowDateTime );
	time += ((uint64_t)file_time.dwHighDateTime) << 32;

	return time / 10000;
}


int mainloop_handle_add(HANDLE h, enum handle_type type, int (*handler)(HANDLE h, void *user), void *user)
{
	struct mainloop_handle_t *mf, *mf_next;

	/*
 	 * Make sure the same handle is not twice in the list
	 */
	
	LIST_FOREACH(mainloop_handle_list, mf, mf_next) {
		if(mf->h == h && !mf->remove) return(-1);
	}

	mf = calloc(sizeof *mf, 1);
	if(mf == NULL) return(-1);
	mf->h       = h;
	mf->handler = handler;
	mf->type    = type;
	mf->user    = user;
	
	LIST_ADD_ITEM(mainloop_handle_list, mf);

	return(0);	
}


/*
 * Mark this handle for removeal
 */
 
int mainloop_handle_del(HANDLE h, enum handle_type type, int (*handler)(HANDLE h, void *user), void *user)
{
	struct mainloop_handle_t *mf, *mf_next;
	int found = 0;
	
	LIST_FOREACH(mainloop_handle_list, mf, mf_next) {
		if( (mf->h == h) && (mf->type == type) && (mf->handler == handler) && (mf->user == user)) {
			mf->remove = 1;
			found = 1;
		}
	}	
	
	if(found == 0) return(-1);
	return(0);
}


int mainloop_timer_add_abs(uint64_t when, uint64_t interval, int (*handler)(void *user), void *user)
{
	struct mainloop_timer_t *mt;
	struct mainloop_timer_t *later;
	struct mainloop_timer_t *sooner;

	/*
	 * Remove this timer if already pending
	 */

	mainloop_timer_del(handler, user);
	
	/* 
	 * Create new instance
	 */
	 	
	mt = calloc(sizeof *mt, 1);
	if(mt == NULL) return(-1);
	mt->interval = interval;
	mt->when     = when;
	mt->handler  = handler;
	mt->user     = user;

	/*
	 * Add it to the list of timers, sorted
	 */
	 
	if(mainloop_timer_list == NULL) {

		mainloop_timer_list = mt;

	} else {
	
		/* 
		 * Find the nodes in the list that come sooner and later then 'mt' 
		 */ 

		sooner = NULL;
		later  = mainloop_timer_list;

		while(later && (later->when < when)) {
			sooner = later;
			later = later->next;
		}		

		/* 
		 * Insert 'mt' between 'sooner' and 'later' 
		 */
				
		mt->prev = sooner;
		mt->next = later;
		if(sooner) sooner->next = mt;
		if(later)  later->prev = mt;
		if(sooner == NULL) mainloop_timer_list = mt;
	}
		
	return(0);	
}


int mainloop_timer_add(uint64_t interval, int (*handler)(void *user), void *user)
{
	uint64_t now = get_time();
	return mainloop_timer_add_abs(now + interval, interval, handler, user);
}
	

/*
 * Mark the given timer for removal
 */
 
int mainloop_timer_del(int (*handler)(void *user), void *user)
{
	struct mainloop_timer_t *mt, *mt_next;
	int found = 0;

	LIST_FOREACH(mainloop_timer_list, mt, mt_next) {
		if( (mt->handler == handler) && (mt->user == user) ) {
			mt->remove = 1;
			found = 1;
		}
			
	}
	
	if(found == 0) return(-1);
	return(0);
}	


#if NEE

/*
 * Signal handler, called for all registered signals
 */

static void mainloop_signal_handler(int signum)
{
	struct mainloop_signal_t *ms, *ms_next;;
	
	LIST_FOREACH(mainloop_signal_list, ms, ms_next) {
		if(ms->signum == signum) ms->signaled = 1;
	}
}



int mainloop_signal_add(int signum, int (*handler)(int signum, void *user), void *user)
{
	struct mainloop_signal_t *ms;
	
	ms = calloc(sizeof *ms, 1);
	if(ms == NULL) return(-1);
	ms->signum  = signum;
	ms->handler = handler;
	ms->user    = user;

	LIST_ADD_ITEM(mainloop_signal_list, ms);
	
	signal(signum, mainloop_signal_handler);
	
	return(0);	
}


/*
 * Mark the given signal for removal
 */
 
int mainloop_signal_del(int signum, int (*handler)(int signum, void *user))
{
	struct mainloop_signal_t *ms, *ms_next;
	int signum_refcount = 0;
	int found = 0;

	LIST_FOREACH(mainloop_signal_list, ms, ms_next) {
		if( ms->signum == signum ) {
			signum_refcount ++;
			if(ms->handler == handler) {
				found = 1;
				ms->remove = 1;
				signum_refcount --;
			}
		}
	}
	
	if(found == 0) return(-1);
	
	/*
	 * If no handlers are registerd for this signal anymore, reset
	 * the signal to default behaviour 
	 */
	 
	if(signum_refcount == 0) {
		signal(signum, SIG_DFL);
	}
	
	return(0);
}

#endif

/*
 * Start mainloop
 */
 
void mainloop_start(void)
{
	mainloop_running = 1;
}


/*
 * Request the mainloop to stop on the next iteration
 */
 
void mainloop_stop(void)
{
	mainloop_running = 0;
}

					
/*
 * Run the mainloop. Stops when mainloop_stop() is called
 */

int mainloop_poll(void)
{
	struct mainloop_handle_t *mf, *mf_next;
	struct mainloop_timer_t  *mt, *mt_next;

	/*
	 * Add fd's to fd_set
	 */

	int n = 0;
	LIST_FOREACH(mainloop_handle_list, mf, mf_next) {
		if(!mf->remove) {
			n ++;
		}
	}

	HANDLE handle_list[n];

	int i = 0;
	LIST_FOREACH(mainloop_handle_list, mf, mf_next) {
		if(!mf->remove) {
			handle_list[i++] = mf->h;
		}
	}
	

	/*
	 * See if there are timers to expire, and set the select()'s
	 * timeout value accordingly
	 */

	uint64_t t_sleep = 0;
	uint64_t now = get_time();
	 
	if(mainloop_timer_list) {
		if(now > mainloop_timer_list->when) {
			t_sleep = 0;
		} else {
			t_sleep = mainloop_timer_list->when - now;
		}
		
	} else {
		t_sleep = 1000;
	}
	

	/* 
	 * The WaitForMultipleObjects call waits on fd's or timeouts. Here the
	 * actual work is done ...
	 */

	if(mainloop_running == 0) return(-1);

	DWORD r = WaitForMultipleObjects(n, handle_list, FALSE, t_sleep);

	/*
	 * Call all register fd that has data
	 */

	if(r == WAIT_FAILED) {
		printf("WSA error %d\n", WSAGetLastError());
		exit(0);
	}

	if(r < WAIT_OBJECT_0 + n) {
		LIST_FOREACH(mainloop_handle_list, mf, mf_next) {
			if(mf->h == handle_list[r - WAIT_OBJECT_0]) {
				mf->handler(mf->h, mf->user);
			}
		}
	}

	/*
	 * Call all timers that have expired
	 */
	
	now = get_time();
	
	while(mainloop_timer_list && (now >= mainloop_timer_list->when)) {

		r = 0;
		if(!mainloop_timer_list->remove) {
			r = mainloop_timer_list->handler(mainloop_timer_list->user);
		}
		
		/*
		 * If the function returned nonzero, reschedule a timer on the
		 * same handler and userdata
		 */
		 
		if(r != 0) {
			mainloop_timer_add_abs(	
					mainloop_timer_list->when + mainloop_timer_list->interval,
					mainloop_timer_list->interval,
					mainloop_timer_list->handler, 
					mainloop_timer_list->user);
		}
		
		/*
		 * Remove and free the expired timer
		 */
		 
		mt_next = mainloop_timer_list->next;
		free(mainloop_timer_list);
		mainloop_timer_list = mt_next;
		if(mainloop_timer_list) mainloop_timer_list->prev = NULL;
	}
	

	/*
	 * Cleanup all fds, signals and timers that have their 'remove' flag set. 
	 */

	LIST_FOREACH(mainloop_handle_list, mf, mf_next) {
		if(mf->remove) {
			LIST_REMOVE_ITEM(mainloop_handle_list, mf);
			free(mf);
		}
	}
#if NEE
	LIST_FOREACH(mainloop_signal_list, ms, ms_next) {
		if(ms->remove) {
			LIST_REMOVE_ITEM(mainloop_signal_list, ms);
			free(ms);
		}
	}
#endif

	LIST_FOREACH(mainloop_timer_list, mt, mt_next) {	
		if(mt->remove) {
			LIST_REMOVE_ITEM(mainloop_timer_list, mt);
			free(mt);
		}
	}

	return(0);
}


/*
 * Run the mainloop. Stops when mainloop_stop() is called
 */

void mainloop_run(void)
{
	mainloop_running  = 1;
	
	while(mainloop_running) {
		mainloop_poll();
	}

}


void mainloop_cleanup(void)
{
	struct mainloop_handle_t     *mf, *mf_next;
	struct mainloop_timer_t  *mt, *mt_next;
	struct mainloop_signal_t *ms, *ms_next;

	if(mainloop_running) return;

	/*
	 * Free all used stuff
	 */

	LIST_FOREACH(mainloop_handle_list,     mf, mf_next) free(mf);
	LIST_FOREACH(mainloop_timer_list,  mt, mt_next) free(mt);
	LIST_FOREACH(mainloop_signal_list, ms, ms_next) free(ms);
	
	mainloop_handle_list     = NULL;
	mainloop_timer_list  = NULL;
	mainloop_signal_list = NULL;

}


/* end */


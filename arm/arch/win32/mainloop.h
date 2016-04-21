/*
 * This program is free software: you can redistribute it and/or modify It
 * under the terms of the GNU Lesser General Public License as published by The
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, But WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * Along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef mainloop_h
#define mainloop_h

#include <windows.h>

/*
 * Mainloop
 */

enum handle_type {
	MAINLOOP_FD_READ,
	MAINLOOP_FD_WRITE,
	MAINLOOP_FD_ERR
};

int mainloop_handle_add(HANDLE h, enum handle_type type, int (*handler)(HANDLE h, void *user), void *user);
int mainloop_handle_del(HANDLE h, enum handle_type type, int (*handler)(HANDLE h, void *user), void *user);

int mainloop_timer_add(uint64_t interval, int (*handler)(void *user), void *user);
int mainloop_timer_del(int (*handler)(void *user), void *user);

void mainloop_start(void);
void mainloop_stop(void);
int  mainloop_poll(void);
void mainloop_run(void);
void mainloop_cleanup(void);

#endif


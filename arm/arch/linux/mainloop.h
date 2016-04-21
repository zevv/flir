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

/*
 * Mainloop
 */

enum fd_type {
	FD_READ,
	FD_WRITE,
	FD_ERR
};

int mainloop_fd_add(int fd, enum fd_type type, int (*handler)(int fd, void *user), void *user);
int mainloop_fd_del(int fd, enum fd_type type, int (*handle)(int fd, void *user), void *user);

int mainloop_timer_add(int sec, int msec, int (*handler)(void *user), void *user);
int mainloop_timer_del(int (*handler)(void *user), void *user);

int mainloop_signal_add(int signum, int (*handler)(int signum, void *user), void *user);
int mainloop_signal_del(int signum, int (*handler)(int signum, void *user));

void mainloop_start(void);
void mainloop_stop(void);
int  mainloop_poll(void);
void mainloop_run(void);
void mainloop_cleanup(void);

#endif /* mainloop_h */


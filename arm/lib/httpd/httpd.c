
// #define DEBUG

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>

#ifdef _WIN32

#include <winsock2.h>
#include <windows.h>
#include "arch/win32/mainloop.h"

typedef SOCKET socket_t;
typedef int socklen_t;

#else

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "arch/linux/mainloop.h"

#define SOCKET_ERROR -1
typedef int socket_t;
typedef void *HANDLE;
#define INVALID_SOCKET -1

#endif

#include "bios/bios.h"
#include "bios/printd.h"
#include "lib/httpd/httpd.h"

#define HANDLER_MAX 8
#define PARAM_MAX 32

struct http_handler {
	char *path;
	http_handler_fn fn;
	void *fndata;
};


struct httpd {
	socket_t sock;
	HANDLE sockh;
	struct http_handler handler_list[HANDLER_MAX];
	int handler_count;
};


struct cgi_param {
	const char *key;
	const char *val;
};


struct http_request {
	char *method;
	char *path;

	struct cgi_param param_list[PARAM_MAX];
	int param_count;
};


struct http_client {
	struct httpd *httpd;
	struct http_request req;
	int headers_sent;
	int code;
	socket_t sock;
	HANDLE sockh;
	struct sockaddr_in sa;
	char rx_buf[4096];
	int rx_len;
};


static int on_read_server(socket_t sock, void *user);
static int on_read_client(socket_t sock, void *user);
static rv on_test(struct http_client *c, struct http_request *req, void *fndata);

#ifdef _WIN32

static int on_read_server_handle(HANDLE sockh, void *user)
{
	struct httpd *h = user;
	WSAResetEvent(h->sockh);
	return on_read_server(0, h);
}

static int on_read_client_handle(HANDLE sockh, void *user)
{
	struct http_client *c = user;
	WSAResetEvent(c->sockh);
	return on_read_client(0, c);
}

#endif


rv httpd_init(uint16_t port, struct httpd **_h)
{
	struct httpd *h;
	int r;

	h = calloc(sizeof *h, 1);
	if(h == NULL) goto err;

	h->sock = socket(AF_INET, SOCK_STREAM, 0);
	if(h->sock == INVALID_SOCKET) goto err;

	struct sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = INADDR_ANY;

	int yes = 1;
	setsockopt(h->sock, SOL_SOCKET, SO_REUSEADDR, (void *)&yes, sizeof(yes));

	r = bind(h->sock, (struct sockaddr *)&sa, sizeof(sa));
	if(r == -1) goto err;

	r = listen(h->sock, SOMAXCONN);
	if(r == -1) goto err;

#ifdef _WIN32
	h->sockh = WSACreateEvent();
	WSAEventSelect(h->sock, h->sockh, FD_READ | FD_ACCEPT);
	mainloop_handle_add(h->sockh, FD_READ, on_read_server_handle, h);
#else
	mainloop_fd_add(h->sock, FD_READ, on_read_server, h);
#endif
	
	*_h = h;
	httpd_register(h, "/test", on_test, NULL);

	return RV_OK;
err:
	if(h) free(h);
	return RV_EIO;
}


rv httpd_exit(struct httpd *h)
{
	free(h);
	return RV_OK;
}


rv httpd_register(struct httpd *h, const char *path, http_handler_fn fn, void *fndata)
{
	if(h->handler_count < HANDLER_MAX) {
		struct http_handler *handler = &h->handler_list[h->handler_count];
		
		handler->path = strdup(path);	
		handler->fn = fn;
		handler->fndata = fndata;

		h->handler_count ++;
		return RV_OK;
	} else {
		return RV_ENOMEM;
	}
}


static int on_read_server(socket_t _, void *user)
{
	struct httpd *h = user;
	struct http_client *c;

	c = calloc(sizeof *c, 1);

	if(c) {
		c->httpd = h;
		c->code = 200;
		socklen_t salen = sizeof(c->sa);
		c->sock = accept(h->sock, (struct sockaddr *)&c->sa, &salen);

		if(c->sock != INVALID_SOCKET) {
#ifdef _WIN32
			c->sockh = WSACreateEvent();
			WSAEventSelect(c->sock, c->sockh, FD_READ);
			mainloop_handle_add(c->sockh, FD_READ, on_read_client_handle, c);
#else
			mainloop_fd_add(c->sock, FD_READ, on_read_client, c);
#endif
		} else {
			free(c);
		}
	}

	return 0;
}


/*
 * Naive HTTP request parser. Only accepts CGI GET requests
 */

static rv parse_request(struct http_request *req, char *buf, size_t len)
{
	char *p;

	if(len < 5) return RV_EINVAL;

	req->method = buf;

	p = strchr(buf, ' ');
	if(p == NULL) return RV_EINVAL;

	*p = '\0';
	req->path = p+1;

	p = strstr(p+1, " HTTP");
	if(p == NULL) return RV_EINVAL;
	*p = '\0';

	/* Check for CGI parameters */

	p = strchr(req->path, '?');
		
	while((req->param_count < PARAM_MAX) && p) {

		*p = '\0';
		p++;

		struct cgi_param *param = &req->param_list[req->param_count];
		param->key = p;
		
		char *pnext = strchr(p, '&');
		if(pnext) {
			*pnext = '\0';
		}

		p = strchr(p, '=');
		if(p) {
			*p = '\0';
			p++;
			param->val = p;
		}

		req->param_count ++;
		p = pnext;
	}

#ifdef DEBUG
	printd("%s %s", req->method, req->path);
	int i;
	for(i=0; i<req->param_count; i++) {
		struct cgi_param *param = &req->param_list[i];
		if(param->val) {
			printd(" %s=%s", param->key, param->val);
		} else {
			printd(" %s", param->key);
		}
	}
	printd("\n");
#endif

	return RV_OK;
}


void httpd_client_set_code(struct http_client *c, int code)
{
	c->code = code;
}


void httpd_client_close(struct http_client *c)
{
#ifdef _WIN32
	mainloop_handle_del(c->sockh, FD_READ, on_read_client_handle, c);
	WSACloseEvent(c->sockh);
	closesocket(c->sock);
#else
	mainloop_fd_del(c->sock, FD_READ, on_read_client, c);
	close(c->sock);
#endif
	free(c);
}


static void httpd_client_writeb(struct http_client *c, const char *data, size_t len)
{
	if(!c->headers_sent) {
		char buf[256];
		int r = snprintf(buf, sizeof(buf), "HTTP/1.1 %d OK\r\n", c->code);
		send(c->sock, buf, r, 0);
		c->headers_sent = 1;
	}

	size_t done = 0;

	while(done < len) {
		ssize_t r = send(c->sock, data, len, 0);
		if(r > 0) {
			done += r;
		} else {
#ifdef _WIN32
			if(WSAGetLastError() != WSAEWOULDBLOCK) break;
#else
			if(errno != EAGAIN) break;
#endif
		}
	}
}


void httpd_client_write(struct http_client *c, const char *data)
{
	httpd_client_writeb(c, data, strlen(data));
}


void httpd_client_printf(struct http_client *c, const char *fmt, ...)
{
	char buf[4096];
	va_list va;

	va_start(va, fmt);
	vsnprintf(buf, sizeof(buf), fmt, va);
	va_end(va);

	httpd_client_write(c, buf);
}


const char *http_request_get_param(struct http_request *req, const char *key)
{
	int i;

	for(i=0; i<req->param_count; i++) {
		struct cgi_param *param = &req->param_list[i];
		if(strcmp(param->key, key) == 0) {
			if(param->val) {
				return param->val;
			} else {
				return "";
			}
		}
	}
	
	return NULL;
}


static void handle_request(struct httpd *h, struct http_client *c)
{
	struct http_request *req = &c->req;
	int i;
	int found = 0;

	/* Check for CGI handler function */

	for(i=0; i<h->handler_count; i++) {
		struct http_handler *handler = &h->handler_list[i];

		if(strcmp(req->path, handler->path) == 0) {
			handler->fn(c, req, handler->fndata);
			found = 1;
		}
	}

	/* Check for static document */
	
	if(found == 0) {
		char fname[256];
		
		snprintf(fname, sizeof(fname), "htdocs/%s", req->path);
		FILE *f = fopen(fname, "rb");

		if(f) {
			if(strstr(fname, ".html")) httpd_client_write(c, "Content-type: text/html\r\n");
			if(strstr(fname, ".jpg")) httpd_client_write(c, "Content-type: image/jpeg\r\n");
			if(strstr(fname, ".png")) httpd_client_write(c, "Content-type: image/png\r\n");
			if(strstr(fname, ".js")) httpd_client_write(c, "Content-type: application/x-javascript\r\n");
			if(strstr(fname, ".css")) httpd_client_write(c, "Content-type: text/css\r\n");
			httpd_client_write(c, "\r\n");

			char buf[4096];
			for(;;) {
				size_t r = fread(buf, 1, sizeof(buf), f);
				httpd_client_writeb(c, buf, r);
				if(r < sizeof(buf)) break;
			}

			fclose(f);
			found = 1;
			httpd_client_close(c);
		}
	}

	/* Not found, generate 404 */

	if(found == 0) {
		httpd_client_set_code(c, 404);
		httpd_client_write(c, "Content-type: text/html\r\n\r\n404 not found\r\n");
		httpd_client_close(c);
	}
}


static int on_read_client(socket_t _, void *user)
{
	struct http_client *c = user;
	ssize_t len;

	len = recv(c->sock, c->rx_buf, sizeof(c->rx_buf)-1, 0);

	if(len > 0) {
		c->rx_buf[len] = '\0';

		rv r = parse_request(&c->req, c->rx_buf, len);
		if(r == RV_OK) {
			handle_request(c->httpd, c);
		}
	} else if(len == 0) {
		httpd_client_close(c);
	} else {
		httpd_client_close(c);
	}

	return 0;
}


static rv on_test(struct http_client *c, struct http_request *req, void *fndata)
{
	httpd_client_write(c, "Content-type: text/html\r\n\r\n");
	httpd_client_write(c, "Hello, world\n");
	httpd_client_close(c);
	return RV_OK;
}


/*
 * End
 */

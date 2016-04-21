#ifndef httpd_h
#define httpd_h

#include "bios/bios.h"

struct http_client;
struct http_request;

typedef rv (*http_handler)(struct http_client *c, struct http_request req);

rv httpd_init(uint16_t port);
rv httpd_register(const char *url, http_handler h);

rv client_write(struct http_client *c, const char *s, size_t len);
rv client_close(struct http_client *c);

#endif

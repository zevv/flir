#ifndef lib_httpd_h
#define lib_httpd_h

struct httpd;
struct http_client;
struct http_request;

typedef rv (*http_handler_fn)(struct http_client *c, struct http_request *req, void *fndata);

rv httpd_init(uint16_t port, struct httpd **h);
rv httpd_exit(struct httpd *h);
rv httpd_register(struct httpd *h, const char *path, http_handler_fn fn, void *fndata);


void httpd_client_set_code(struct http_client *c, int code);
void httpd_client_close(struct http_client *c);
void httpd_client_write(struct http_client *c, const char *data);
void httpd_client_printf(struct http_client *c, const char *fmt, ...);

const char *http_request_get_param(struct http_request *req, const char *key);

#endif

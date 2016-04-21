#ifndef drv_socketcand_can_h
#define drv_socketcand_can_h

#include "drv/can/can.h"

#ifdef _WIN32
typedef SOCKET socket_t;
#else
typedef void *HANDLE;
typedef int socket_t;
#endif

struct drv_can_socketcand_data {
	const char *host;
	const char *ifname;

	socket_t sock;
	HANDLE sockh;
};

extern const struct drv_can drv_can_socketcand;

#endif


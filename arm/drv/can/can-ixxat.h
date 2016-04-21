#ifndef drv_can_ixxat_h
#define drv_can_ixxat_h

#include "drv/can/can.h"

struct drv_can_ixxat_data {
	HANDLE hdevice;
	HANDLE hcontrol;
	HANDLE hchannel;
};

extern const struct drv_can drv_can_ixxat;

#endif


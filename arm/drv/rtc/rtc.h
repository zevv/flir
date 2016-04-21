#ifndef drv_rtc_h
#define drv_rtc_h

#include <stddef.h>
#include "config.h"
#include "bios/rtc.h"

#ifdef DRV_RTC
#define DEV_INIT_RTC(name, drv, ...) DEV_INIT(RTC, rtc, name, drv,__VA_ARGS__) 
#else
#define DEV_INIT_RTC(name, drv, ...)
#endif

struct dev_rtc {
	const struct drv_rtc *drv;
	void *drv_data;
	rv init_status;
};

struct drv_rtc {
	rv (*init)(struct dev_rtc *rtc);
	rv (*get)(struct dev_rtc *rtc, struct rtc_time *t);
};

#endif


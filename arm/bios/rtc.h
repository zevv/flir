#ifndef bios_rtc_h
#define bios_rtc_h

#include <stdint.h>

struct rtc_time {
	uint16_t year;
	uint8_t mon;
	uint8_t mday;
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
};

struct dev_rtc;

rv rtc_init(struct dev_rtc *dev);
rv rtc_get(struct dev_rtc *dev, struct rtc_time *t);

#endif

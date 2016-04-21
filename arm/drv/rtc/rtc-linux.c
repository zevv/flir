
#include <time.h>
#include <stddef.h>

#include "bios/rtc.h"
#include "drv/rtc/rtc.h"
#include "bios/printd.h"


static rv init(struct dev_rtc *rtc)
{
	return RV_OK;
}



static void get(struct dev_rtc *rtc, struct rtc_time *t)
{
	time_t t_now;
	struct tm *tm;

	t_now = time(NULL);
	tm = gmtime(&t_now);

	t->year = tm->tm_year + 1900;
	t->mon  = tm->tm_mon + 1;
	t->mday = tm->tm_mday;
	t->hour = tm->tm_hour;
	t->min  = tm->tm_min;
	t->sec  = tm->tm_sec;
}


const struct drv_rtc drv_rtc_linux = {
	.init = init,
	.get = get,
};

/*
 * End
 */

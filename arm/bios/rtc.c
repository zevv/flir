
#include <stdint.h>

#include "bios/bios.h"
#include "bios/dev.h"
#include "bios/rtc.h"
#include "bios/cmd.h"
#include "bios/printd.h"
#include "drv/rtc/rtc.h"
#include "plat.h"


rv rtc_init(struct dev_rtc *dev);
{
	return dev->drv->init(dev);
}



void rtc_get(struct dev_rtc *dev, struct rtc_time *t)
{
	dev->drv->get(dev, t);
}



#ifdef DEFAULT_RTC_DEV

static rv on_cmd_rtc(uint8_t argc, char **argv)
{
	struct rtc_time t;
	rtc_get(&rtc0, &t);
	printd("%04d-%02d-%02d %02d:%02d:%02d\n", 
			t.year, t.mon, t.mday,
			t.hour, t.min, t.sec);

	return RV_OK;
}

CMD_REGISTER(rtc, on_cmd_rtc, "g[et]");

#endif

/*
 * End
 */



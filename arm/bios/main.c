
#include <stdio.h>

#include "config.h"

#include "bios/bios.h"

#include "bios/adc.h"
#include "bios/arch.h"
#include "bios/button.h"
#include "bios/can.h"
#include "bios/cmd.h"
#include "bios/dev.h"
#include "bios/eeprom.h"
#include "bios/evq.h"
#include "bios/fan.h"
#include "bios/gpio.h"
#include "bios/led.h"
#include "bios/mmc.h"
#include "bios/printd.h"
#include "bios/relay.h"
#include "bios/rtc.h"
#include "bios/spi.h"
#include "bios/uart.h"
#include "bios/sysinfo.h"

#include "drv/adc/adc.h"
#include "drv/button/button.h"
#include "drv/can/can.h"
#include "drv/eeprom/eeprom.h"
#include "drv/fan/fan.h"
#include "drv/gpio/gpio.h"
#include "drv/led/led.h"
#include "drv/mmc/mmc.h"
#include "drv/relay/relay.h"
#include "drv/rtc/rtc.h"
#include "drv/spi/spi.h"
#include "drv/uart/uart.h"

#include "lib/config.h"
#include "lib/atoi.h"

typedef int mainret;
mainret main(void);

void dev_init(struct dev_descriptor *desc, void *ptr);

/*
 * This macro typecasts the device descriptor 'dev' to the proper
 * dev_### struct type and calls the drv init function when the
 * device state is RV_ENOTREADY.
 */

/*lint -e9026 -e9023*/
#define DO_INIT(t1, t2)                                                    \
	if(desc->type == DEV_TYPE_ ## t1) {                                \
		struct dev_ ## t2 *dev = desc->dev;                        \
		if(dev->init_status == RV_ENOTREADY) {                     \
			dev->init_status = t2 ## _init(dev);               \
		}                                                          \
	}


void dev_init(struct dev_descriptor *desc, void *ptr)
{

#ifdef DRV_GPIO
	DO_INIT(GPIO, gpio);
#endif
#ifdef DRV_LED
	DO_INIT(LED, led);
#endif
#ifdef DRV_SPI
	DO_INIT(SPI, spi);
#endif
#ifdef DRV_ADC
	DO_INIT(ADC, adc);
#endif
#ifdef DRV_BUTTON
	DO_INIT(BUTTON, button);
#endif
#ifdef DRV_CAN
	DO_INIT(CAN, can);
#endif
#ifdef DRV_EEPROM
	DO_INIT(EEPROM, eeprom);
#endif
#ifdef DRV_RTC
	DO_INIT(RTC, rtc);
#endif
#ifdef DRV_FAN
	DO_INIT(FAN, fan);
#endif
#ifdef DRV_RELAY
	DO_INIT(RELAY, relay);
#endif
#ifdef DRV_MMC
	DO_INIT(MMC, mmc);
#endif

}


mainret main(void)
{

	/* Init architecture */

	arch_init();

	/* Special case: early uart init to allow proper debugging output */

	uart_init(&uart0);

	/* Init drivers. We run the initialisation cycle a number of times in a
	 * row to make sure that drivers with dependencies all get a chance to
	 * initialize */

	uint8_t i;
	for(i=0; i<5u; i++) {
		dev_foreach(DEV_TYPE_ANY, dev_init, NULL);
	}

	//sysinfo_init();

	/* Push boot event and run main loop */

	event_t ev;
	ev.type = EV_BOOT;
	evq_push(&ev);

	for(;;) {
		evq_wait(&ev);
	}
}


const char *rv_str(rv r)
{
	const char *msg;
	switch(r) {
		case RV_OK:        msg = "No error"; break;
		case RV_ENOENT:    msg = "Not found"; break;
		case RV_EIO:       msg = "I/O error"; break;
		case RV_ENODEV:    msg = "No such device"; break;
		case RV_EIMPL:     msg = "Not implemented"; break;
		case RV_ETIMEOUT:  msg = "Timeout"; break;
		case RV_EBUSY:     msg = "Busy"; break;
		case RV_EINVAL:    msg = "Invalid argument"; break;
		case RV_ENOTREADY: msg = "Not ready"; break;
		case RV_ENOSPC:    msg = "No space left"; break;
		case RV_EPROTO:    msg = "Protocol error"; break;
		case RV_ENOMEM:    msg = "Out of memory"; break;
		default:           msg = "Unknown error"; break;
	}
	return msg;
}


static rv on_cmd_version(uint8_t argc, char **argv)
{
	printd("%s v%s %s@%s (gcc %s) %s %s\n", 
			ARCH,
			VERSION_SVN, 
			BUILD_USER, BUILD_HOSTNAME,
			__VERSION__,
			__DATE__, __TIME__);
	return RV_OK;
}


CMD_REGISTER(version, on_cmd_version, "");


static rv on_cmd_mem(uint8_t argc, char **argv)
{
	rv r = RV_EINVAL;

	if(argc > 0u) {
		char cmd = argv[0][0];

		if((cmd == 'd') && (argc >= 3u)) {

			uintptr_t addr = (uint32_t)a_to_s32(argv[1]);
			size_t len = (size_t)a_to_s32(argv[2]);

			hexdump((void *)addr, len, addr);

			r = RV_OK;
		}
	}

	return r;
}

CMD_REGISTER(mem, on_cmd_mem, "d[ump] <addr> <len>");

/*
 * End
 */


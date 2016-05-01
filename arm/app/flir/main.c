
#include <string.h>

#include "bios/bios.h"
#include "bios/i2c.h"
#include "bios/arch.h"
#include "bios/button.h"
#include "bios/uart.h"
#include "bios/printd.h"
#include "bios/gpio.h"
#include "bios/cmd.h"
#include "bios/evq.h"
#include "bios/led.h"
#include "bios/can.h"
#include "bios/spi.h"

#include "lib/config.h"
#include "lib/atoi.h"

#include "arch/lpc/inc/chip.h"
#include "arch/lpc/inc/usbd.h"

#include "lepton.h"

typedef struct _USB_DEVICE_INFO {
	uint16_t DevType;
	uint32_t DevDetailPtr;
} USB_DEV_INFO;


typedef struct _USBD {
	void (*init_clk_pins)(void);
	void (*isr)(void);
	void (*init)( const USB_DEV_INFO * DevInfoPtr );
	void (*connect)(uint32_t con);
} USBD;


typedef struct _ROM {
	const USBD * pUSBD;
} ROM;


typedef struct _HID_DEVICE_INFO {
	uint16_t idVendor;
	uint16_t idProduct;
	uint16_t bcdDevice;
	uint32_t StrDescPtr;
	uint8_t InReportCount;
	uint8_t OutReportCount;
	uint8_t SampleInterval;
	void (*InReport)( uint8_t src[], uint32_t length);
	void (*OutReport)(uint8_t dst[], uint32_t length);
} HID_DEVICE_INFO;


static void GetInReport(uint8_t src[], uint32_t length);
static void SetOutReport(uint8_t dst[], uint32_t length);

static const uint8_t USB_StringDescriptor[] = {

	0x04, USB_STRING_DESCRIPTOR_TYPE, WBVAL(0x0409),

	/* Index 0x04: Manufacturer */

	0x08, USB_STRING_DESCRIPTOR_TYPE,
	'F',0, 'L',0, 'I',0, 'R',0,

	/* Index 0x20: Product */

	0x28,USB_STRING_DESCRIPTOR_TYPE,
	'N',0, 'X',0, 'P',0, ' ',0, 'L',0, 'P',0, 'C',0, '1',0, '3',0, 'X',0,
	'X',0, ' ',0, 'H',0, 'I',0, 'D',0, ' ',0, ' ',0, ' ',0, ' ',0,

	/* Index 0x48: Serial Number */

	0x1A, USB_STRING_DESCRIPTOR_TYPE,
	'D',0, 'E',0, 'M',0, 'O',0, '0',0, '0',0, '0',0, '0',0, '0',0, '0',0,
	'0',0, '0',0,

	/* Index 0x62: Interface 0, Alternate Setting 0 */

	0x0E, USB_STRING_DESCRIPTOR_TYPE,
	'H',0, 'I',0, 'D',0, ' ',0, ' ',0, ' ',0,
};


static const HID_DEVICE_INFO HidDevInfo = {
	.idVendor = 0x1234,
	.idProduct = 0x5678,
	.bcdDevice = 0x9abc,
	.StrDescPtr = (uint32_t) &USB_StringDescriptor[0],
	.InReportCount = 64,
	.OutReportCount = 64,
	.SampleInterval = 1,
	.InReport = GetInReport,
	.OutReport = SetOutReport,
};


static const USB_DEV_INFO DeviceInfo = {
	.DevType = USB_DEVICE_CLASS_HUMAN_INTERFACE,
	.DevDetailPtr = (uint32_t)&HidDevInfo,
};


static ROM **rom = (ROM **)0x1fff1ff8;

uint8_t printd_buf[63] = "Hallo\n";
uint8_t printd_ptr = 6;
int flop;

void flir_tx(uint8_t c)
{
	if(printd_ptr < sizeof(printd_buf)) {
		printd_buf[printd_ptr++] = c;
		printd_buf[printd_ptr] = 0;
	}
}


void usb_irq(void)
{
	(*rom)->pUSBD->isr();
}


static void GetInReport(uint8_t src[], uint32_t len)
{
	static uint8_t line = 0;
	if(len > 63) len = 63;

	if(printd_ptr > 0) {
		src[0] = 0xff;
		memcpy(&src[1], printd_buf, len);
		printd_ptr = 0;
	} else {
		src[0] = line;
		memcpy(src+1, img[line], len);
		line++;
		if(line == 60) line = 0;
	}

	if(line == 0) {
		static int n = 0;
		static struct dev_led *leds[] = { &led0, &led1, &led2, &led3, &led4, &led5, &led6, &led7 };
		for(int i=0; i<8; i++) led_set(leds[i], abs(n) == i);
		if(++n == 8) n = -6;
	}
}


static void SetOutReport(uint8_t dst[], uint32_t length)
{
	event_t ev;
	ev.type = EV_UART;
	ev.uart.data = dst[0];
	evq_push(&ev);
	led_set(&led1, LED_STATE_ON);
}



static void on_ev_boot(event_t *ev, void *data)
{
	(void)led_set(&led0, LED_STATE_BLINK);
	printd("FLIR\n");

	/* Connect USB on olimex board */

	gpio_set_pin_direction(&gpio0, 6, GPIO_DIR_OUTPUT);
	gpio_set_pin(&gpio0, 6, 1);

	/* Setup ROM descriptors */

	//Chip_USB_Init();

	// 2. Enable 32-bit timer 1 (CT32B1) and IOCONFIG block:

	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_CT32B1);
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_IOCON);
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_USB);

	// 3. Initialize USB clock and pins:

	(*rom)->pUSBD->init_clk_pins();

	// 4. Set up device type and information:

	volatile int n;
	for (n = 0; n < 75; n++);

	(*rom)->pUSBD->init(&DeviceInfo);
	(*rom)->pUSBD->connect(1);

	/* Configure lepton */

	lepton_set_16(LEPTON_MOD_AGC, LEPTON_CMD_ID_AGC, 0);
}


EVQ_REGISTER(EV_BOOT, on_ev_boot);


static void on_ev_button(event_t *ev, void *data)
{
	static uint8_t agc = 0;

	if(ev->button.state == BUTTON_STATE_PUSH) {

		if(ev->button.dev == &but1) {
			agc = 1-agc;
			lepton_set_16(LEPTON_MOD_AGC, LEPTON_CMD_ID_AGC, agc);
			led_set(&led7, agc);
		}

		if(ev->button.dev == &but2) {
			lepton_run(LEPTON_MOD_SYS, LEPTON_CMD_ID_RUN_FCC);
		}
	}

	flop = (int)ev;

	printd("%d\n", flop);
}

EVQ_REGISTER(EV_BUTTON, on_ev_button);

/*
 * End
  */

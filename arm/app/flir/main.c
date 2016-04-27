
#include <string.h>

#include "bios/bios.h"
#include "bios/arch.h"
#include "bios/uart.h"
#include "bios/printd.h"
#include "bios/gpio.h"
#include "bios/cmd.h"
#include "bios/evq.h"
#include "bios/led.h"
#include "bios/can.h"
#include "bios/spi.h"

#include "lib/config.h"

#include "arch/lpc/inc/chip.h"
#include "arch/lpc/inc/usbd.h"

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
static uint8_t img[60][63];


void usb_irq(void)
{
	(*rom)->pUSBD->isr();
}


static void GetInReport(uint8_t src[], uint32_t length)
{
	static uint8_t line = 0;

	src[0] = line;
	memcpy(&src[1], img[line], 63);

	line = (line + 1) % 60;
}


static void SetOutReport(uint8_t dst[], uint32_t length)
{
}



static void on_ev_boot(event_t *ev, void *data)
{
	(void)led_set(&led_green, LED_STATE_ON);
	(void)led_set(&led_yellow, LED_STATE_BLINK);
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
}


EVQ_REGISTER(EV_BOOT, on_ev_boot);

static uint16_t vmin = 0xffff;
static uint16_t vmax = 0x0000;
static uint16_t vmin2 = 0xffff;
static uint16_t vmax2 = 0x0000;

static void lepton_read_packet(void)
{
	uint16_t buf[82];

	spi_read(&spi0, buf, sizeof(buf));

	uint16_t id = buf[0] & 0x0fff;

	if(id < 60) {

		if(id == 59) {
			vmin2 = vmin;
			vmax2 = vmax;
			vmin = 0xffff;
			vmax = 0x0000;
		}
		
		uint16_t *p = (void *)buf + 4;

		uint8_t x;
		for(x=0; x<63; x++) {

			uint16_t v = *p++;

			if(v < vmin) vmin = v;
			if(v > vmax) vmax = v;

			uint16_t dv = vmax2 - vmin2;
			if(dv > 0) {
				int16_t w = 255 * (v - vmin2) / dv;
				if(w < 0) w = 0;
				if(w > 255) w = 255;
				img[id][x] = w;
			}

		}
	}
}


static void lepton_read_frame(void)
{
	uint8_t i;

	for(i=0; i<80; i++) {
		lepton_read_packet();
	}
}


static void on_ev_tick(event_t *ev, void *data)
{
	static uint8_t n = 0;
	
	if(++n == 3) {
		lepton_read_frame();
		n = 0;
	}
}

EVQ_REGISTER(EV_TICK_100HZ, on_ev_tick);


static rv on_cmd_usb(uint8_t argc, char **argv)
{
	return RV_OK;
}

CMD_REGISTER(usb, on_cmd_usb, "f[req]");

/*
 * End
 */

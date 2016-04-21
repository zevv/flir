
#include <string.h>

#include "bios/bios.h"
#include "bios/printd.h"
#include "bios/gpio.h"
#include "bios/cmd.h"
#include "bios/evq.h"
#include "bios/led.h"
#include "bios/can.h"

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
	void (*init)( USB_DEV_INFO * DevInfoPtr );
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



static uint8_t USB_StringDescriptor[] = {

	0x04, USB_STRING_DESCRIPTOR_TYPE, WBVAL(0x0409),

	/* Index 0x04: Manufacturer */

	0x1C, USB_STRING_DESCRIPTOR_TYPE,
	'N',0, 'X',0, 'P',0,' ',0,'S',0, 'E',0, 'M',0,'I',0, 'C',0,'O',0,'N',0, 'D',0,' ',0,

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


static ROM **rom = (ROM **)0x1fff1ff8;
static USB_DEV_INFO DeviceInfo;
static HID_DEVICE_INFO HidDevInfo;


void usb_irq(void)
{
	(*rom)->pUSBD->isr();
}


void GetInReport(uint8_t src[], uint32_t length)
{
	static int i = 0;
	src[0] = i++;;
}


void SetOutReport(uint8_t dst[], uint32_t length)
{
	led_set(&led7, dst[0]);
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

	HidDevInfo.idVendor = 0x1234;
	HidDevInfo.idProduct = 0x5678;
	HidDevInfo.bcdDevice = 0x9abc;
	HidDevInfo.StrDescPtr = (uint32_t) &USB_StringDescriptor[0];
	HidDevInfo.InReportCount = 64;
	HidDevInfo.OutReportCount = 64;
	HidDevInfo.SampleInterval = 1;
	HidDevInfo.InReport = GetInReport;
	HidDevInfo.OutReport = SetOutReport;

	DeviceInfo.DevType = USB_DEVICE_CLASS_HUMAN_INTERFACE;
	DeviceInfo.DevDetailPtr = (uint32_t)&HidDevInfo;

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

static void on_ev_tick(event_t *ev, void *data)
{
	printd("click\n");
}

EVQ_REGISTER(EV_BUTTON, on_ev_tick);


static rv on_cmd_usb(uint8_t argc, char **argv)
{
	return RV_OK;
}

CMD_REGISTER(usb, on_cmd_usb, "f[req]");

/*
 * End
 */

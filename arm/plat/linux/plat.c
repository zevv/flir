
#include <stdint.h>
#include <stddef.h>

#include "plat.h"
#include "bios/bios.h"

#include "drv/drv.h"
#include "drv/adc/adc-dummy.h"
#include "drv/uart/uart-linux.h"
#include "drv/can/can-linux.h"
#include "drv/gpio/gpio-dummy.h"
#include "drv/eeprom/eeprom-stdio.h"
#include "drv/rtc/rtc-linux.h"
#include "drv/mmc/mmc-stdio.h"
#include "drv/relay/relay-gpio.h"
#include "drv/button/button-linux.h"
#include "drv/led/led-linux.h"

extern struct dev_gpio gpio_dummy;

DEV_INIT_GPIO(gpio_dummy, drv_gpio_dummy)

DEV_INIT_LED(led_green, drv_led_linux, 
	.y = 1, 
	.x = 1, 
	.color = 0x00ff00,
)

DEV_INIT_LED(led_yellow, drv_led_linux, 
	.y = 1, 
	.x = 2, 
	.color = 0xffff00,
)

DEV_INIT_LED(led_red, drv_led_linux, 
	.y = 1, 
	.x = 3, 
	.color = 0xff0000,
)

DEV_INIT_UART(uart0, drv_uart_linux, 
	.dev = "/dev/tty",
	.baudrate = 115200,
)

DEV_INIT_CAN(can0, drv_can_linux, 
	.ifname = "vcan1",
	.fd = 0,
)

DEV_INIT_CAN(can1, drv_can_linux, 
	.ifname = "slcan0",
	.fd = 0,
)

DEV_INIT_EEPROM(eeprom0, drv_eeprom_stdio, 
	.fname = "/tmp/eeprom0.bin",
	.size = 4096
)

DEV_INIT_MMC(mmc0, drv_mmc_stdio, 
	.fname = "/tmp/mmc0.bin",
	.size = 32 * 1024 * 1024
)

DEV_INIT_RTC(rtc0, drv_rtc_linux)

DEV_INIT_RELAY(relay_main_high, drv_relay_gpio,
	.dev_gpio = &gpio_dummy
)

DEV_INIT_RELAY(relay_main_low, drv_relay_gpio,
	.dev_gpio = &gpio_dummy
)

DEV_INIT_RELAY(relay_precharge, drv_relay_gpio,
	.dev_gpio = &gpio_dummy
)

DEV_INIT_RELAY(relay_reset, drv_relay_gpio,
	.dev_gpio = &gpio_dummy
)

DEV_INIT_BUTTON(button_on, drv_button_linux)

DEV_INIT_BUTTON(button_off, drv_button_linux)

DEV_INIT_BUTTON(button_precharge, drv_button_linux)

DEV_INIT_BUTTON(button_reset, drv_button_linux)

DEV_INIT_ADC(supply_voltage, drv_adc_dummy)

DEV_INIT_ADC(master_bus_voltage, drv_adc_dummy)

DEV_INIT_ADC(slave_bus_voltage, drv_adc_dummy)

/*
 * End
 */

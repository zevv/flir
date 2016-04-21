
#include <windows.h>

#include <stdint.h>
#include <stddef.h>

#include "plat.h"
#include "bios/bios.h"

#include "drv/drv.h"
#include "drv/adc/adc-dummy.h"
#include "drv/uart/uart-win32.h"
#include "drv/can/can-ixxat.h"
#include "drv/gpio/gpio-dummy.h"
#include "drv/led/led-gpio.h"
#include "drv/mmc/mmc-stdio.h"
#include "drv/button/button-gpio.h"
#include "drv/relay/relay-gpio.h"
#include "drv/eeprom/eeprom-stdio.h"

extern struct dev_gpio gpio_dummy;


DEV_INIT_GPIO(gpio0, drv_gpio_dummy)


DEV_INIT_LED(led_green, drv_led_gpio,
	.dev_gpio = &gpio0,
)

DEV_INIT_LED(led_yellow, drv_led_gpio,
	.dev_gpio = &gpio0,
)

DEV_INIT_LED(led_red, drv_led_gpio,
	.dev_gpio = &gpio0,
)

DEV_INIT_UART(uart0, drv_uart_win32);

DEV_INIT_CAN(can0, drv_can_ixxat)

DEV_INIT_CAN(can1, drv_can_ixxat)

DEV_INIT_EEPROM(eeprom0, drv_eeprom_stdio, 
	.fname = "/tmp/eeprom0.bin",
	.size = 4096
)

DEV_INIT_MMC(mmc0, drv_mmc_stdio, 
	.fname = "t:\\mmc0.bin",
	.size = 32 * 1024 * 1024
)

DEV_INIT_BUTTON(button_on, drv_button_gpio,
	.dev_gpio = &gpio0,
)

DEV_INIT_BUTTON(button_off, drv_button_gpio,
	.dev_gpio = &gpio0,
)

DEV_INIT_BUTTON(button_precharge, drv_button_gpio,
	.dev_gpio = &gpio0,
)

DEV_INIT_BUTTON(button_reset, drv_button_gpio,
	.dev_gpio = &gpio0,
)

DEV_INIT_RELAY(relay_main_high, drv_relay_gpio,
	.dev_gpio = &gpio0
)

DEV_INIT_RELAY(relay_main_low, drv_relay_gpio,
	.dev_gpio = &gpio0
)

DEV_INIT_RELAY(relay_precharge, drv_relay_gpio,
	.dev_gpio = &gpio0
)

DEV_INIT_RELAY(relay_reset, drv_relay_gpio,
	.dev_gpio = &gpio0
)

DEV_INIT_ADC(supply_voltage, drv_adc_dummy)

DEV_INIT_ADC(master_bus_voltage, drv_adc_dummy)

DEV_INIT_ADC(slave_bus_voltage, drv_adc_dummy)


/*
 * End
 */

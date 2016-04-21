
#include <stdint.h>
#include <stddef.h>

#include "bios/bios.h"
#include "plat.h"

#include "drv/drv.h"

#include "drv/drv.h"
#include "drv/adc/adc-avr.h"
#include "drv/uart/uart-avr.h"
#include "drv/gpio/gpio-avr.h"
#include "drv/led/led-gpio.h"


DEV_INIT_GPIO(gpio_led_green, drv_gpio_avr,
	.port = &PORTA,
	.pin = 1,
)

DEV_INIT_GPIO(gpio_led_yellow, drv_gpio_avr,
	.port = &PORTA,
	.pin = 2
)

DEV_INIT_GPIO(gpio_led_red, drv_gpio_avr,
	.port = &PORTA,
	.pin = 3
)


DEV_INIT_LED(led_green, drv_led_gpio,
	.dev_gpio = &gpio_led_green,
)

DEV_INIT_LED(led_yellow, drv_led_gpio,
	.dev_gpio = &gpio_led_yellow,
)

DEV_INIT_LED(led_red, drv_led_gpio,
	.dev_gpio = &gpio_led_red,
)

DEV_INIT_UART(uart0, drv_uart_avr,
	.uart = 0,
)


DEV_INIT_ADC(adc0, drv_adc_avr, 
	.channel = 0
)


/*
 * End
 */

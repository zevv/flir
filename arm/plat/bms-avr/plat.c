
#include <stdint.h>
#include <stddef.h>

#include "bios/bios.h"
#include "plat.h"

#include "drv/drv.h"

#include "drv/drv.h"
#include "drv/adc/adc-avr.h"
#include "plat/bms-avr/uart-bitbang.h"
#include "drv/gpio/gpio-avr.h"
#include "drv/led/led-gpio.h"
#include "drv/spi/spi-avr.h"
#include "drv/gpio/gpio-spi.h"
#include "drv/mmc/mmc-spi.h"


DEV_INIT_GPIO(gpio_a, drv_gpio_avr,
	.port = &PORTA,
)

DEV_INIT_GPIO(gpio_b, drv_gpio_avr,
	.port = &PORTB,
)

DEV_INIT_SPI(spi0, drv_spi_avr)

DEV_INIT_GPIO(gpio_spi, drv_gpio_spi,
	.spi_dev = &spi0,
	.clock_gpio = &gpio_b,
	.clock_pin = 4,
)


DEV_INIT_LED(led_green, drv_led_gpio,
	.dev_gpio = &gpio_a,
	.pin = 1,
)

DEV_INIT_LED(led_yellow, drv_led_gpio,
	.dev_gpio = &gpio_a,
	.pin = 2,
)

DEV_INIT_LED(led_red, drv_led_gpio,
	.dev_gpio = &gpio_a,
	.pin = 3,
)

DEV_INIT_UART(uart0, drv_uart_bitbang)


DEV_INIT_ADC(adc_avr, drv_adc_avr, 
	.channel = 0
)


DEV_INIT_MMC(mmc0, drv_mmc_spi,

	.dev_gpio_cs = &gpio_b,
	.cs_gpio_pin = 4,
	
	.dev_gpio_detect = &gpio_b,
	.detect_gpio_pin = 2,

	.dev_spi = &spi0,
)


/*
 * End
 */

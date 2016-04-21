
#include "plat.h"
#include "bios/bios.h"

#include "drv/drv.h"
#include "drv/led/led-gpio.h"
#include "drv/button/button-gpio.h"
#include "drv/gpio/gpio-lpc.h"
#include "drv/gpio/gpio.h"
#include "drv/uart/uart-lpc.h"

const uint32_t OscRateIn = 12000000;


/*lint -e9029 -e9078 -e923 -e9075 */

DEV_INIT_GPIO(gpio0, drv_gpio_lpc,
	.port = 0,
)

DEV_INIT_GPIO(gpio1, drv_gpio_lpc,
	.port = 1,
)

DEV_INIT_GPIO(gpio2, drv_gpio_lpc,
	.port = 2,
)

DEV_INIT_GPIO(gpio3, drv_gpio_lpc,
	.port = 3,
)

DEV_INIT_LED(led0, drv_led_gpio,
	.dev_gpio = &gpio3,
	.pin = 0,
)

DEV_INIT_LED(led1, drv_led_gpio,
	.dev_gpio = &gpio3,
	.pin = 1,
)

DEV_INIT_LED(led2, drv_led_gpio,
	.dev_gpio = &gpio3,
	.pin = 2,
)

DEV_INIT_LED(led3, drv_led_gpio,
	.dev_gpio = &gpio3,
	.pin = 3,
)

DEV_INIT_LED(led4, drv_led_gpio,
	.dev_gpio = &gpio2,
	.pin = 4,
)

DEV_INIT_LED(led5, drv_led_gpio,
	.dev_gpio = &gpio2,
	.pin = 5,
)

DEV_INIT_LED(led6, drv_led_gpio,
	.dev_gpio = &gpio2,
	.pin = 6,
)

DEV_INIT_LED(led7, drv_led_gpio,
	.dev_gpio = &gpio2,
	.pin = 7,
)

DEV_INIT_BUTTON(but1, drv_button_gpio,
	.dev_gpio = &gpio2,
	.pin = 9,
)

DEV_INIT_BUTTON(but2, drv_button_gpio,
	.dev_gpio = &gpio1,
	.pin = 4,
)

DEV_INIT_UART(uart0, drv_uart_lpc,
)


/*
 * End
 */

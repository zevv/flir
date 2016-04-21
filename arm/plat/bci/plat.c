
#include "plat.h"
#include "bios/bios.h"

#include "drv/drv.h"
#include "drv/led/led-gpio.h"
#include "drv/gpio/gpio-stm32.h"
#include "drv/gpio/gpio.h"
#include "drv/adc/adc-stm32.h"
#include "drv/uart/uart-stm32.h"
#include "drv/can/can-stm32.h"
#include "drv/eeprom/eeprom-at25.h"
#include "drv/relay/relay-gpio.h"
#include "drv/button/button-gpio.h"
#include "drv/spi/spi-stm32.h"
#include "drv/mmc/mmc-spi.h"

/*lint -e9029 -e9078 -e923 -e9075 */

DEV_INIT_GPIO(gpio_a, drv_gpio_stm32,
	.port = GPIOA,
)

DEV_INIT_GPIO(gpio_b, drv_gpio_stm32,
	.port = GPIOB,
)

DEV_INIT_GPIO(gpio_c, drv_gpio_stm32,
	.port = GPIOC,
)



DEV_INIT_LED(led_green, drv_led_gpio,
	.dev_gpio = &gpio_b,
	.pin = 11,
)

DEV_INIT_LED(led_yellow, drv_led_gpio,
	.dev_gpio = &gpio_b,
	.pin = 15,
)

DEV_INIT_LED(led_red, drv_led_gpio,
	.dev_gpio = &gpio_b,
	.pin = 14,
)


DEV_INIT_UART(uart0, drv_uart_stm32,
	.uart = 0,
)


DEV_INIT_CAN(can0, drv_can_stm32,
	.bus = CAN1,
)

DEV_INIT_CAN(can1, drv_can_stm32,
	.bus = CAN2,
)


DEV_INIT_SPI(spi0, drv_spi_stm32,
	.spi = SPI1,
)


DEV_INIT_EEPROM(eeprom0, drv_eeprom_at25,
	.size = 32768,
	.dev_gpio_cs = &gpio_a,
	.cs_gpio_pin = 2,
	.dev_spi = &spi0,
)

DEV_INIT_RELAY(relay_main_high, drv_relay_gpio,
	.dev_gpio = &gpio_c,
	.pin = 0,
)

DEV_INIT_RELAY(relay_main_low, drv_relay_gpio,
	.dev_gpio = &gpio_c,
	.pin = 1,
)

DEV_INIT_RELAY(relay_precharge, drv_relay_gpio,
	.dev_gpio = &gpio_c,
	.pin = 2,
)

DEV_INIT_RELAY(relay_reset, drv_relay_gpio,
	.dev_gpio = &gpio_c,
	.pin = 3,
)


DEV_INIT_BUTTON(button_on, drv_button_gpio,
	.dev_gpio = &gpio_b,
	.pin = 5
)

DEV_INIT_BUTTON(button_off, drv_button_gpio,
	.dev_gpio = &gpio_b,
	.pin = 6
)

DEV_INIT_BUTTON(button_precharge, drv_button_gpio,
	.dev_gpio = &gpio_b,
	.pin = 7
)

DEV_INIT_BUTTON(button_reset, drv_button_gpio,
	.dev_gpio = &gpio_b,
	.pin = 8
)


DEV_INIT_ADC(supply_voltage, drv_adc_stm32,
	.id = 2
)

DEV_INIT_MMC(mmc0, drv_mmc_spi,

	.dev_gpio_cs = &gpio_c,
	.cs_gpio_pin = 4,
	
	.dev_gpio_detect = &gpio_b,
	.detect_gpio_pin = 2,

	.dev_spi = &spi0,
)


/*
 * End
 */

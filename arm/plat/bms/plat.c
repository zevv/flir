
#include <stdint.h>
#include <stddef.h>

#include "plat.h"

#include "drv/drv.h"

#include "drv/arm/led.h"
#include "drv/arm/uart.h"
#include "drv/arm/can.h"
#include "drv/arm/eeprom.h"
#include "drv/arm/fan.h"


DEV_INIT_LED(led_green, drv_led_stm32,
	.port = GPIO_PORT_B,
	.pin = 11
)

DEV_INIT_LED(led_yellow, drv_led_stm32,
	.port = GPIO_PORT_B,
	.pin = 15
)

DEV_INIT_LED(led_red, drv_led_stm32,
	.port = GPIO_PORT_B,
	.pin = 14
)

DEV_INIT_UART(uart0, drv_uart_stm32,
	.uart = 0,
)

DEV_INIT_CAN(can0, drv_can_stm32,
	.bus = 0,
)

DEV_INIT_EEPROM(eeprom0, drv_eeprom_stm32)


DEV_INIT_FAN(fan0, drv_fan_stm32)


/*
 * End
 */

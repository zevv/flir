
#include <stdint.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#include "bios/bios.h"
#include "bios/uart.h"
#include "drv/uart/uart-avr.h"
#include "bios/evq.h"


static void init(struct dev_uart *dev)
{
}


static rv set_speed(struct dev_uart *uart, uint16_t baudrate)
{
	return RV_OK;
}


static rv tx(struct dev_uart *uart, uint8_t c)
{
        uint8_t s;

        s = SREG; cli();

        DDRA |= (1<<PA2);

        uint16_t v = (c << 1) | 0x200;

        uint8_t b1 = PORTA |  (1<<PA2);
        uint8_t b0 = PORTA & ~(1<<PA2);
        uint8_t i;
        for(i=0; i<10; i++) {
                PORTA = (v & 1) ? b1 : b0;
                v >>= 1;
                __asm__("nop");
                __asm__("nop");
                __asm__("nop");
                __asm__("nop");
                __asm__("nop");
                __asm__("nop");
        }
        PORTA |= (1<<PA2);
        SREG = s;

	return RV_OK;
}


struct drv_uart drv_uart_bitbang = {
	.init = init,
	.set_speed = set_speed,
	.tx = tx,
};

/*
 * End
 */

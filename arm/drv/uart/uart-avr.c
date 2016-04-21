
#include <stdint.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#include "bios/bios.h"
#include "bios/uart.h"
#include "drv/uart/uart-avr.h"
#include "bios/evq.h"

#define RB_SIZE 80

struct ringbuffer_t {
	uint8_t head;
	uint8_t tail;
	uint8_t buf[RB_SIZE];
};

static volatile struct ringbuffer_t rb = { 0, 0 };


static rv init(struct dev_uart *dev)
{
	rb.head = rb.tail = 0;

	/* Enable receiver and transmitter and rx interrupts */

	UCSRB = (1<<RXCIE) | (1<<RXEN) | (1<<TXEN);


	return RV_OK;
}


static rv set_speed(struct dev_uart *uart, uint16_t baudrate)
{
	uint16_t div = ((F_CPU / baudrate) / 16 - 1);
	UBRRH = (div >> 8);
	UBRRL = (div & 0xff);

	return RV_OK;
}


static rv tx(struct dev_uart *uart, uint8_t c)
{
	uint8_t head;
	head = (rb.head + 1) % RB_SIZE;

	while(rb.tail == head);

	rb.buf[rb.head] = c;
	rb.head = head;

	/* Enable TX-reg-empty interrupt */

	UCSRB |= (1<<UDRIE);

	return RV_OK;
}


ISR(USART_RXC_vect)
{
	char c = UDR;
	event_t ev;
	ev.type = EV_UART;
	ev.uart.data = c;
	evq_push(&ev);
}


ISR(USART_UDRE_vect)
{
        UDR = rb.buf[rb.tail];
        rb.tail = (rb.tail + 1) % RB_SIZE;

        if(rb.tail == rb.head) {
                UCSRB &= ~(1<<UDRIE);
        }
}

struct drv_uart drv_uart_avr = {
	.init = init,
	.set_speed = set_speed,
	.tx = tx,
};

/*
 * End
 */

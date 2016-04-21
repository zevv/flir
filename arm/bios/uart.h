#ifndef bios_uart_h
#define bios_uart_h

#include <stdint.h>

struct dev_uart;

rv uart_init(struct dev_uart *dev);
rv uart_tx(uint8_t c);
rv uart_set_speed(struct dev_uart *uart, uint16_t baudrate);
rv uart_tx2(struct dev_uart *uart, uint8_t c);

#endif

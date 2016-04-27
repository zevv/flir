#ifndef plat_h
#define plat_h

void plat_init(void);

extern struct dev_button but1;
extern struct dev_button but2;
extern struct dev_gpio gpio0;
extern struct dev_gpio gpio1;
extern struct dev_gpio gpio2;
extern struct dev_gpio gpio3;
extern struct dev_arch arch0;
extern struct dev_led led0;
extern struct dev_led led1;
extern struct dev_led led2;
extern struct dev_led led3;
extern struct dev_led led4;
extern struct dev_led led5;
extern struct dev_led led6;
extern struct dev_led led7;
extern struct dev_uart uart0;
extern struct dev_spi spi0;
extern struct dev_i2c i2c0;

#define DEFAULT_UART_DEV uart0

#define led_green led0
#define led_red led1
#define led_yellow led2

#endif

#ifndef plat_h
#define plat_h

void plat_init(void);

extern struct dev_arch arch0;
extern struct dev_led led_green;
extern struct dev_led led_yellow;
extern struct dev_led led_red;
extern struct dev_uart uart0;
extern struct dev_can can0;
extern struct dev_eeprom eeprom0;
extern struct dev_fan fan0;
extern struct dev_adc adc0;

#define DEFAULT_ARCH_DEV arch0
#define DEFAULT_UART_DEV uart0
#define DEFAULT_CAN_DEV can0
#define DEFAULT_EEPROM_DEV eeprom0

#endif

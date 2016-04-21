#ifndef plat_h
#define plat_h

void plat_init(void);

extern struct dev_gpio gpio_c;
extern struct dev_arch arch0;
extern struct dev_led led_green;
extern struct dev_led led_yellow;
extern struct dev_led led_red;
extern struct dev_uart uart0;
extern struct dev_can can0;
extern struct dev_can can1;
extern struct dev_spi spi0;
extern struct dev_eeprom eeprom0;
extern struct dev_fan fan0;
extern struct dev_relay relay_main_high;
extern struct dev_relay relay_main_low;
extern struct dev_relay relay_precharge;
extern struct dev_relay relay_reset; //EN_RECOVERY
extern struct dev_button button_on;
extern struct dev_button button_off;
extern struct dev_button button_precharge;
extern struct dev_button button_reset;
extern struct dev_adc supply_voltage;
extern struct dev_adc master_bus_voltage;
extern struct dev_adc slave_bus_voltage;
extern struct dev_mmc mmc0;

#define DEFAULT_UART_DEV uart0
#define DEFAULT_CAN_DEV can0
#define DEFAULT_EEPROM_DEV eeprom0
#define DEFAULT_MMC_DEV mmc0

#endif

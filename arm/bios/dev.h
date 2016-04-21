
#ifndef dev_bios_h
#define dev_bios_h

enum dev_type {
	DEV_TYPE_ANY,

	DEV_TYPE_LED,
	DEV_TYPE_UART,
	DEV_TYPE_ADC,
	DEV_TYPE_BUTTON,
	DEV_TYPE_CAN,
	DEV_TYPE_EEPROM,
	DEV_TYPE_RTC,
	DEV_TYPE_FAN,
	DEV_TYPE_RELAY,
	DEV_TYPE_SPI,
	DEV_TYPE_GPIO,
	DEV_TYPE_MMC,

	DEV_TYPE_MAX,
};

struct dev_descriptor {
	const char *name;
	void *dev;
	enum dev_type type;
};

typedef void (*dev_iterator)(struct dev_descriptor *d, void *ptr);

void dev_foreach(enum dev_type type, dev_iterator cb, void *ptr);
struct dev_descriptor *dev_find(const char *name, enum dev_type type);

#endif

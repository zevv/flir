#ifndef bios_gpio_h
#define bios_gpio_h

#include <stdint.h>
#include <stddef.h>

typedef enum {
	GPIO_DIR_INPUT,
	GPIO_DIR_OUTPUT,
} gpio_direction;

typedef uint8_t gpio_pin;

struct dev_gpio;

rv gpio_init(struct dev_gpio *dev);
rv gpio_set_pin_direction(struct dev_gpio *dev, gpio_pin pin, gpio_direction dir);
rv gpio_set_pin(struct dev_gpio *dev, gpio_pin pin, uint8_t onoff);
rv gpio_get_pin(struct dev_gpio *dev, gpio_pin pin, uint8_t *onoff);
uint8_t gpio_get_pin_count(struct dev_gpio *dev);

rv gpio_set(struct dev_gpio *dev, uint32_t val, uint32_t mask);

#endif

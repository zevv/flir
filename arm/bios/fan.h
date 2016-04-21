#ifndef bios_fan_h
#define bios_fan_h

#include <stdint.h>
#include <stddef.h>

struct dev_fan;

rv fan_init(struct dev_fan *dev);
void fan_set_speed(struct dev_fan *fan, uint8_t speed);

#endif

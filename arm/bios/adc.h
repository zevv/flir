#ifndef bios_adc_h
#define bios_adc_h

#include <stdint.h>


enum adc_id_t {
	SUPPLY_VOLTAGE,
	MASTER_BUS_VOLTAGE,
	SLAVE_BUS_VOLTAGE,
};

struct dev_adc;

rv adc_init(struct dev_adc *dev);
rv adc_read(struct dev_adc *dev, int16_t *val);

int16_t adc_continuous_read(struct dev_adc *dev, enum adc_id_t adc_id);

#endif


#ifndef drv_adc_h
#define drv_adc_h

#include <stdint.h>
#include <stddef.h>
#include <bios/adc.h>

#include "config.h"
#include "bios/adc.h"

#ifdef DRV_ADC
#define DEV_INIT_ADC(name, drv, ...) DEV_INIT(ADC, adc, name, drv, __VA_ARGS__) 
#else
#define DEV_INIT_ADC(name, drv, ...) 
#endif

struct dev_adc {
	const struct drv_adc *drv;
	void *drv_data;
	rv init_status;
	int16_t value;
	enum adc_id_t adc_id;
};

struct drv_adc {
	rv (*init)(struct dev_adc *adc);
	void (*re_init)(void);
	rv (*read)(struct dev_adc *dev, int16_t *val);
	int16_t (*continuous_read) (struct dev_adc *dev, uint8_t id);
};

#endif

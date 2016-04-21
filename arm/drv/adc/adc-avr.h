
#ifndef drv_avr_adc_h
#define drv_avr_adc_h

#include "drv/adc/adc.h"

struct drv_adc_avr_data {
	uint8_t channel;
};

extern const struct drv_adc drv_adc_avr;

#endif

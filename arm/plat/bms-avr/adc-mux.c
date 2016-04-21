
#include <mux/io.h>

#include "bios/bios.h"
#include "bios/adc.h"
#include "bios/printd.h"
#include "drv/adc/adc-mux.h"
#include "bios/printd.h"


static void init(struct dev_adc *dev)
{
	ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
}


static rv readh(struct dev_adc *dev, int16_t *val)
{
	struct drv_adc_mux_data *dd = dev->drv_data;

	ADMUX = dd->channel;
	ADCSRA |= (1<<ADSC);
	while(!(ADCSRA & (1<<ADIF)));
	*val = ADC;

	return RV_OK;
}


const struct drv_adc drv_adc_mux = {
	.init = init,
	.read = readh,
};

/*
 * End
 */

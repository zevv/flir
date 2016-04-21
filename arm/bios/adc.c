
#include "bios/bios.h"
#include "bios/dev.h"
#include "bios/adc.h"
#include "drv/adc/adc.h"
#include "bios/printd.h"


rv adc_init(struct dev_adc *dev)
{
	return dev->drv->init(dev);
}


rv adc_read(struct dev_adc *dev, int16_t *val)
{
	rv r;

	if(dev->drv->read != NULL) {
		r = dev->drv->read(dev, &dev->value);
		*val = dev->value;
	} else {
		r = RV_EIMPL;
	}

	return r;
}


int16_t adc_continuous_read(struct dev_adc *dev, enum adc_id_t adc_id)
{
	if(adc_id == SUPPLY_VOLTAGE) {
		dev->value = dev->drv->continuous_read(dev, 2);
	}
	if(adc_id == MASTER_BUS_VOLTAGE) {
		dev->value = dev->drv->continuous_read(dev, 0);
	}
	if(adc_id == SLAVE_BUS_VOLTAGE) {
		dev->value = dev->drv->continuous_read(dev, 1);
	}
	return dev->value;
}

/*
 * End
 */

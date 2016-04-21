
#include "bios/bios.h"
#include "bios/adc.h"
#include "drv/adc/adc.h"
#include "bios/printd.h"


static rv init(struct dev_adc *dev)
{
	return RV_OK;
}


static rv readh(struct dev_adc *dev, int16_t *val)
{
	return RV_EIMPL;
}


static int16_t continuous_read(struct dev_adc *dev, uint8_t id)
{
	return 0;
}


const struct drv_adc drv_adc_dummy = {
	.init = init,
	.read = readh,
	.continuous_read = continuous_read,
};

/*
 * End
 */

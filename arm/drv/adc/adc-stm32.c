
#include "bios/bios.h"
#include "bios/adc.h"
#include "drv/adc/adc-stm32.h"

#include "bios/evq.h"
#include "bios/printd.h"

#include "stm32f1xx_hal.h"

#define N_ADC_DEV 3

uint8_t count;
uint16_t ADC_linear_value[3];
uint16_t ADC_real_value[3];

static rv init(struct dev_adc *dev)
{
	RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
	ADC1->CR1 |= ADC_CR1_EOCIE;
	ADC1->CR2 |= ADC_CR2_ADON | ADC_CR2_CAL;
	while ( !(ADC1-> CR2 & ADC_CR2_CAL) );
	ADC1->SQR3 = 1;
	ADC1->CR2 |= ADC_CR2_ADON;
	NVIC_EnableIRQ (ADC1_2_IRQn);

	return RV_OK;
}

static void re_init(void)
{
	uint8_t ADC_conversion_factor[] = {9,9,23};
	ADC_real_value[count] = ADC_linear_value[count] * ADC_conversion_factor[count];
	switch(count){
		case 0:{
			count=(count+1)%N_ADC_DEV;
			ADC1->CR2 |= ADC_CR2_ADON;
			ADC1->CR1 |= ADC_CR1_EOCIE;
			ADC1->SQR3 = 8;
			ADC1->CR2 |= ADC_CR2_ADON;
			break;
		}
		case 1:{
			count=(count+1)%N_ADC_DEV;
			ADC1->CR2 |= ADC_CR2_ADON;
			ADC1->CR1 |= ADC_CR1_EOCIE;
			ADC1->SQR3 = 9;
			ADC1->CR2 |= ADC_CR2_ADON;
			break;
		}
		case 2:{
			count=(count+1)%N_ADC_DEV;
			ADC1->CR2 |= ADC_CR2_ADON;
			ADC1->CR1 |= ADC_CR1_EOCIE;
			ADC1->SQR3 = 1;
			ADC1->CR2 |= ADC_CR2_ADON;
			break;
		}
	}
}

static rv read (struct dev_adc *dev, int16_t *val)
{
	struct drv_adc_stm32_data *dd = dev->drv_data;
	*val = ADC_real_value[dd->id];
	return RV_OK;
}

static int16_t continuous_read (struct dev_adc *dev, uint8_t id)
{
	return ADC_real_value[id];
}

void adc1_2_irq(void)
{
	if( (ADC1->SR & ADC_SR_EOC)  ) {
		ADC_linear_value[count] = (ADC1->DR);
		ADC1->SR &= ~ ADC_SR_EOC;
		ADC1->CR2 &= ~ ADC_CR2_ADON;
		ADC1->CR1 &= ~ ADC_CR1_EOCIE;
		ADC1->CR2 &= ~ ADC_CR2_TSVREFE;
		re_init();
	}
}

struct drv_adc drv_adc_stm32 = {
	.init = init,
	.re_init = re_init,
	.read = read,
	.continuous_read = continuous_read
};

/*
 * End
 */


















#ifndef drv_stm32_spi_h
#define drv_stm32_spi_h

#include "drv/spi/spi.h"
#include "stm32f1xx_hal.h"

struct drv_spi_stm32_data {

	SPI_TypeDef *spi;
	SPI_HandleTypeDef _hspi;

};

extern struct drv_spi drv_spi_stm32;

#endif


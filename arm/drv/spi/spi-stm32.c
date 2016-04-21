
/*
 * Driver for EEPROM connected to SPI bus
 */

#include <stdint.h>

#include "arch/stm32/arch.h"
#include "bios/bios.h"
#include "bios/printd.h"
#include "drv/spi/spi-stm32.h"

#include "stm32f1xx_hal.h"


void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{

	if (hspi->Instance == SPI1)
	{
		__HAL_RCC_SPI1_CLK_ENABLE();
		if(0) RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;

		GPIO_InitTypeDef GPIO_InitStruct;

		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
		GPIO_InitStruct.Pin = GPIO_PIN_5;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

		GPIO_InitStruct.Pull = GPIO_PULLDOWN;
		GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
		GPIO_InitStruct.Pin = GPIO_PIN_6;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_INPUT;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
		GPIO_InitStruct.Pin = GPIO_PIN_7;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	}
}


static rv init(struct dev_spi *dev)
{
	struct drv_spi_stm32_data *dd = dev->drv_data;

	if(dd->spi == SPI1) {


		SPI_HandleTypeDef *hspi = &dd->_hspi;;

		hspi->Instance = SPI1;
		hspi->State = HAL_SPI_STATE_RESET;
		hspi->Init.Direction = SPI_DIRECTION_2LINES;
		hspi->Init.Mode = SPI_MODE_MASTER;
		hspi->Init.DataSize = SPI_DATASIZE_8BIT;
		hspi->Init.CLKPolarity = SPI_POLARITY_HIGH;
		hspi->Init.CLKPhase = SPI_PHASE_2EDGE;
		hspi->Init.NSS = SPI_NSS_SOFT;
		hspi->Init.TIMode = SPI_TIMODE_DISABLE;
		hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
		hspi->Init.FirstBit = SPI_FIRSTBIT_MSB;
		hspi->Init.CRCPolynomial = 7;
		hspi->Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;

		HAL_SPI_Init(hspi);
		__HAL_SPI_ENABLE(hspi);
	}

	if(dd->spi == SPI2) {
		printd("SPI2 not yet implemented\n");
	}

	return RV_OK;
}


static rv readh(struct dev_spi *dev, void *buf, size_t len)
{
	struct drv_spi_stm32_data *dd = dev->drv_data;

	HAL_StatusTypeDef r = HAL_SPI_Receive(&dd->_hspi, buf, len, 10);
	return hal_status_to_rv(r);
}


static rv writeh(struct dev_spi *dev, const void *buf, size_t len)
{
	struct drv_spi_stm32_data *dd = dev->drv_data;
	
	HAL_StatusTypeDef r = HAL_SPI_Transmit(&dd->_hspi, (void *)buf, len, 10);
	return hal_status_to_rv(r);
}


static rv read_write(struct dev_spi *dev, void *buf, size_t len)
{
	struct drv_spi_stm32_data *dd = dev->drv_data;

	HAL_StatusTypeDef r = HAL_SPI_TransmitReceive(&dd->_hspi, buf, buf, len, 10);
	return hal_status_to_rv(r);
}


struct drv_spi drv_spi_stm32 = {
	.init = init,
	.read = readh,
	.write = writeh,
	.read_write = read_write,
};

/*
 * End
 */

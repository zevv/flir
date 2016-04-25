
/*
 * Driver for EEPROM connected to SPI bus
 */

#include <stdint.h>

#include "arch/lpc/arch.h"
#include "bios/bios.h"
#include "bios/printd.h"
#include "bios/led.h"
#include "drv/spi/spi-lpc.h"

#include "chip.h"


static const PINMUX_GRP_T pinmuxing[] = {
	{(uint32_t)IOCON_PIO0_2,  (IOCON_FUNC1 | IOCON_RESERVED_BIT_6 | IOCON_RESERVED_BIT_7 | IOCON_MODE_PULLUP)},/* PIO0_2 used for SSEL */
	{(uint32_t)IOCON_PIO0_8,  (IOCON_FUNC1 | IOCON_RESERVED_BIT_6 | IOCON_RESERVED_BIT_7 | IOCON_MODE_PULLUP)}, /* PIO0_8 used for MISO */
	{(uint32_t)IOCON_PIO0_9,  (IOCON_FUNC1 | IOCON_RESERVED_BIT_6 | IOCON_RESERVED_BIT_7 | IOCON_MODE_PULLUP)}, /* PIO0_9 used for MOSI */
	{(uint32_t)IOCON_PIO2_11, (IOCON_FUNC1 | IOCON_RESERVED_BIT_6 | IOCON_RESERVED_BIT_7 | IOCON_MODE_PULLUP)}, /* PIO2_11 used for SCK */
};


static rv init(struct dev_spi *dev)
{
	Chip_SYSCTL_PeriphReset(RESET_SSP0);
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_SSP0);

        Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_IOCON);
	Chip_IOCON_PinLocSel(LPC_IOCON, IOCON_SCKLOC_PIO2_11);
	Chip_IOCON_SetPinMuxing(LPC_IOCON, pinmuxing, sizeof(pinmuxing) / sizeof(PINMUX_GRP_T));

	Chip_SSP_Init(LPC_SSP0);
	Chip_SSP_SetBitRate(LPC_SSP0, 3 * 1000 * 1000);
	Chip_SSP_SetFormat(LPC_SSP0, SSP_BITS_8, SSP_FRAMEFORMAT_SPI, SSP_CLOCK_CPHA1_CPOL1);
	Chip_SSP_SetMaster(LPC_SSP0, 1);
	Chip_SSP_Enable(LPC_SSP0);
	
	Chip_GPIO_WriteDirBit(LPC_GPIO_PORT, 2, 10, 1);

	NVIC_EnableIRQ(SSP0_IRQn);

	return RV_OK;
}


static void (*fn_cb)(void) = NULL;
static Chip_SSP_DATA_SETUP_T xf_setup;


void ssp_irq(void)
{
	led_set(&led3, LED_STATE_ON);

	Chip_SSP_Int_Disable(LPC_SSP0);

	Chip_SSP_Int_RWFrames8Bits(LPC_SSP0, &xf_setup);

	if ((xf_setup.rx_cnt != xf_setup.length) || (xf_setup.tx_cnt != xf_setup.length)) {
		Chip_SSP_Int_Enable(LPC_SSP0);
	} else {
		led_set(&led4, LED_STATE_ON);
		Chip_GPIO_WritePortBit(LPC_GPIO_PORT, 2, 10, 1);
		if(fn_cb != NULL) {
			fn_cb();
			fn_cb = NULL;
		}
	}
}




static rv readh(struct dev_spi *dev, void *buf, size_t len)
{
	Chip_GPIO_WritePortBit(LPC_GPIO_PORT, 2, 10, 0);
	Chip_SSP_ReadFrames_Blocking(LPC_SSP0, buf, len);
	Chip_GPIO_WritePortBit(LPC_GPIO_PORT, 2, 10, 1);

	return RV_OK;
}


static rv read_async(struct dev_spi *dev, void *buf, size_t len, void (*fn)(void))
{
	//struct drv_spi_lpc_data *dd = dev->drv_data;
	
	(void)led_set(&led2, LED_STATE_ON);

	rv r;

	if(fn_cb == NULL) {

		xf_setup.length = len;
		xf_setup.tx_data = buf;
		xf_setup.rx_data = buf;
		xf_setup.rx_cnt = 0;
		xf_setup.tx_cnt = 0;

		fn_cb = fn;
		
		Chip_GPIO_WritePortBit(LPC_GPIO_PORT, 2, 10, 0);

		Chip_SSP_Int_FlushData(LPC_SSP0);
		Chip_SSP_Int_RWFrames8Bits(LPC_SSP0, &xf_setup);

		r = RV_OK;
	} else {
		r = RV_EBUSY;
	}

	return r;
}


static rv writeh(struct dev_spi *dev, const void *buf, size_t len)
{
	return RV_OK;
}


static rv read_write(struct dev_spi *dev, void *buf, size_t len)
{
	return RV_OK;
}


struct drv_spi drv_spi_lpc = {
	.init = init,
	.read = readh,
	.read_async = read_async,
	.write = writeh,
	.read_write = read_write,
};

/*
 * End
 */


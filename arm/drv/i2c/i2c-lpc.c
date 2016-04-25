
#include <stdint.h>
#include <string.h>

#include "arch/lpc/arch.h"
#include "bios/bios.h"
#include "bios/led.h"
#include "drv/i2c/i2c-lpc.h"

#include "chip.h"

STATIC const PINMUX_GRP_T pinmuxing[] = {
	{(uint32_t)IOCON_PIO0_4,  (IOCON_FUNC1 | IOCON_RESERVED_BIT_6 | IOCON_RESERVED_BIT_7 | IOCON_FASTI2C_EN)}, /* PIO0_4 used for SCL */
	{(uint32_t)IOCON_PIO0_5,  (IOCON_FUNC1 | IOCON_RESERVED_BIT_6 | IOCON_RESERVED_BIT_7 | IOCON_FASTI2C_EN)}, /* PIO0_5 used for SDA */
};


static rv init(struct dev_i2c *dev)
{
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_I2C);
	Chip_SYSCTL_DeassertPeriphReset(RESET_I2C0);

        Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_IOCON);
	Chip_IOCON_SetPinMuxing(LPC_IOCON, pinmuxing, sizeof(pinmuxing) / sizeof(PINMUX_GRP_T));

	Chip_I2C_Init(I2C0);
	Chip_I2C_SetClockRate(I2C0, 100 * 1000);
	Chip_I2C_SetMasterEventHandler(I2C0, Chip_I2C_EventHandlerPolling);
	
	return RV_OK;
}

	
static rv xfer(struct dev_i2c *i2c, uint8_t addr, const void *txbuf, size_t txlen, void *rxbuf, size_t rxlen)
{
	static I2C_XFER_T xfer = {0};

	xfer.slaveAddr = addr;
	xfer.txBuff = txbuf;
	xfer.txSz = txlen;
	xfer.rxBuff = rxbuf;
	xfer.rxSz = rxlen;

	while (Chip_I2C_MasterTransfer(I2C0, &xfer) == I2C_STATUS_ARBLOST) {}

	return RV_OK;
}


struct drv_i2c drv_i2c_lpc = {
	.init = init,
	.xfer = xfer,
};

/*
 * End
 */

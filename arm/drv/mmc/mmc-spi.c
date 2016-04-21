
#include <assert.h>
#include <string.h>

#include "bios/bios.h"
#include "bios/evq.h"
#include "bios/dev.h"
#include "bios/mmc.h"
#include "bios/spi.h"
#include "bios/gpio.h"
#include "drv/mmc/mmc-spi.h"
#include "bios/printd.h"

#include "lib/fatfs/ff.h"

#define CMD0    (0)             /* GO_IDLE_STATE */
#define CMD1    (1)             /* SEND_OP_COND (MMC) */
#define CMD8    (8)             /* SEND_IF_COND */
#define CMD9    (9)             /* SEND_CSD */
#define CMD10   (10)            /* SEND_CID */
#define CMD12   (12)            /* STOP_TRANSMISSION */
#define CMD16   (16)            /* SET_BLOCKLEN */
#define CMD17   (17)            /* READ_SINGLE_BLOCK */
#define CMD18   (18)            /* READ_MULTIPLE_BLOCK */
#define CMD23   (23)            /* SET_BLOCK_COUNT (MMC) */
#define CMD24   (24)            /* WRITE_BLOCK */
#define CMD25   (25)            /* WRITE_MULTIPLE_BLOCK */
#define CMD32   (32)            /* ERASE_ER_BLK_START */
#define CMD33   (33)            /* ERASE_ER_BLK_END */
#define CMD38   (38)            /* ERASE */
#define CMD55   (55)            /* APP_CMD */
#define CMD58   (58)            /* READ_OCR */

#define ACMD13   (0x80 | 13)     /* SD_STATUS (SDC) */
#define ACMD23   (0x80 | 23)     /* SET_WR_BLK_ERASE_COUNT (SDC) */
#define ACMD41   (0x80 | 41)     /* SEND_OP_COND (SDC) */

#define SD_RSP_IDLE            0x01
#define SD_RSP_ILLEGAL_CMD     0x04


static void mmc_poll(struct dev_descriptor *desc, void *ptr);


static void mmc_set_status(struct dev_mmc *dev, mmc_status status, rv reason)
{
	struct drv_mmc_spi_data *dd = dev->drv_data;

	if(status != dd->_status) {

		struct mmc_card_info *info = &dd->_card_info;

		if(status == MMC_STATUS_READY) {
			const char *type = "?";
			if(info->type == MMC_CARD_V1) { type = "1"; }
			if(info->type == MMC_CARD_V2) { type = "2"; }
			if(info->type == MMC_CARD_V2_BLOCK) { type = "2b"; }
			if(info->type == MMC_CARD_V3) { type = "3"; }
			printd("MMCv%s, %d Mb (%d sectors of %d bytes)\n", 
					type, 
					(info->sector_count / (1024u * 1024u)) * info->block_size,
					info->sector_count, info->block_size);
		} else {
			printd("MMC: %s\n", rv_str(reason));
		}

		dd->_status = status;
	}
}


static void cs(struct drv_mmc_spi_data *dd, uint8_t v)
{
	(void)gpio_set_pin(dd->dev_gpio_cs, dd->cs_gpio_pin, v);
}


static uint8_t xchg(struct drv_mmc_spi_data *dd, uint8_t v)
{
	spi_read_write(dd->dev_spi, &v, 1);
	return v;
}



static uint8_t sd_crc7(uint8_t* chr, uint8_t cnt, uint8_t crc)
{
	uint8_t i, a;
	uint8_t d;

	for(a = 0; a < cnt; a++){          
		d = chr[a];          
		for(i = 0; i < 8; i++){               
			crc <<= 1;
			if( (d & 0x80) ^ (crc & 0x80) ) {crc ^= 0x09;}               
			d <<= 1;
		}
	}     
	return crc & 0x7F;
} 

static uint8_t mmc_send_cmd(struct drv_mmc_spi_data *dd, uint8_t cmd, uint32_t arg)
{

	uint8_t s;
	uint16_t n = 100;

	do {
		s = xchg(dd, 0xff);
		n --;
	} while ((s != 0xff) && (n > 0));


	uint8_t buf[6] = {
		0x40 | (cmd & 0x7f), /* command */
		arg >> 24,           /* arguments */
		arg >> 16,
		arg >>  8,
		arg >>  0,
	};

	/* calc crc */

	buf[5] = (sd_crc7(buf, 5, 0) << 1) | 1;
	
	/* write command */

	spi_write(dd->dev_spi, &buf, sizeof(buf));

	/* wait for response */

	n = 1000;
	do {
		s = xchg(dd, 0xff);
		n --;
	} while ((s & 0x80) && (n > 0));
	
	if(dd->_debug) {
		printd("CMD%d(%08x): %d\n", cmd & 0x7F, arg, s);
	}

	return s;
}


static uint8_t mmc_cmd_rsp(struct drv_mmc_spi_data *dd, uint8_t cmd, uint32_t arg, uint8_t *rsp, size_t rsp_len)
{
	uint8_t s;

	cs(dd, 0);
	s = mmc_send_cmd(dd, cmd, arg);
	spi_read_write(dd->dev_spi, rsp, rsp_len);

	uint8_t dummy[2] = { 0xff, 0xff };
	spi_read_write(dd->dev_spi, dummy, sizeof(dummy));

	cs(dd, 1);

	if(dd->_debug) {
		hexdump(rsp, rsp_len, 0);
	}

	return s;
}


static uint8_t mmc_cmd_data(struct drv_mmc_spi_data *dd, uint8_t cmd, uint32_t arg, uint8_t *buf, size_t len)
{
	uint8_t s;

	memset(buf, 0xff, len);

	cs(dd, 0);
	s = mmc_send_cmd(dd, cmd, arg);

	/* Wait for read token */

	uint16_t n = 100;
	do {
		s = xchg(dd, 0xff);
		n --;
	} while ((s == 0xFF) && (n > 0));

	/* Read data if token found */

	if(s == 0xFE) {
		spi_read_write(dd->dev_spi, buf, len);
		uint8_t crc[2] = { 0xff, 0xff };
		spi_read_write(dd->dev_spi, crc, sizeof(crc));
	}

	cs(dd, 1);
	
	if(dd->_debug) {
		hexdump(buf, len, 0);
	}

	return s == 0xfe;
}


static uint8_t mmc_cmd_data_write(struct drv_mmc_spi_data *dd, uint8_t cmd, uint32_t arg, const uint8_t *buf, size_t len)
{
	uint8_t s;

	cs(dd, 0);
	s = mmc_send_cmd(dd, cmd, arg);

	/* Wait for card ready */

	uint16_t n = 100;
	do {
		s = xchg(dd, 0xff);
		n --;
	} while ((s == 0xFF) && (n > 0));

	/* Write token */
		
	s = xchg(dd, 0xfe);

	/* Write data */

	if(s == 0xFF) {
		spi_write(dd->dev_spi, buf, len);
		uint8_t crc[2] = { 0xff, 0xff };
		spi_write(dd->dev_spi, crc, sizeof(crc));
	}

	cs(dd, 1);
	
	return s == 0xFF;
}


static uint8_t mmc_cmd(struct drv_mmc_spi_data *dd, uint8_t cmd, uint32_t arg)
{
	uint8_t s;

	cs(dd, 0);
	s = mmc_send_cmd(dd, cmd, arg);
	cs(dd, 1);

	return s;
}


static uint8_t mmc_cmd_and_wait(struct drv_mmc_spi_data *dd, uint8_t cmd)
{
	uint8_t i = 32;
	uint8_t s;

	do {
		if(cmd & 0x80) {
			(void)mmc_cmd(dd, CMD55, 0);
		}
		s = mmc_cmd(dd, cmd, 1ul<<30);
		i--;
	} while(s != 0 && i > 0);

	return (i > 0);
}


static rv init(struct dev_mmc *dev)
{
	struct drv_mmc_spi_data *dd = dev->drv_data;
	rv r = RV_OK;

	r = gpio_set_pin_direction(dd->dev_gpio_cs, dd->cs_gpio_pin, GPIO_DIR_OUTPUT);
	
	if(r == RV_OK) {
		r = gpio_set_pin_direction(dd->dev_gpio_detect, dd->detect_gpio_pin, GPIO_DIR_OUTPUT);
	}

	if(r == RV_OK) {
		cs(dd, 1);
	}

	if(r == RV_OK) {
		dev_foreach(DEV_TYPE_MMC, mmc_poll, NULL);
	}

	return r;
}


/*
 * http://elm-chan.org/docs/mmc/gfx1/sdinit.png
 */

void mmc_detect(struct dev_mmc *dev)
{
	struct drv_mmc_spi_data *dd = dev->drv_data;

	dd->_debug = 0;

	/* Send 80 dummy clocks to wake up card */

	uint8_t dummy[8] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	spi_write(dd->dev_spi, &dummy, sizeof(dummy));

	rv r = RV_EBUSY;
	uint8_t s;

	/* MMC idle state */

	s = mmc_cmd(dd, CMD0, 0);

	if(s == 1) {

		/* Check for 0x1AA */
			
		uint8_t ocr[4];
		s = mmc_cmd_rsp(dd, CMD8, 0x1AA, ocr, sizeof(ocr));

		if(s == 1) {

			if (ocr[2] == 0x01 && ocr[3] == 0xAA) {
		
				s = mmc_cmd_and_wait(dd, ACMD41);

				if(s == 0) {
					r = RV_EIO;
					goto out;
				}

				s = mmc_cmd_rsp(dd, CMD58, 0x1AA, ocr, sizeof(ocr));

				if(s == 0) {
					if(ocr[0] & 0x40) {
						dd->_card_info.type = MMC_CARD_V2_BLOCK;
						r = RV_OK;
					} else {
						dd->_card_info.type = MMC_CARD_V2;
						r = RV_OK;
					}

				} else {
					r = RV_EIO;
					goto out;
				}
					

			} else {
				r = RV_EIO;
			} 
		} else {

			s = mmc_cmd_and_wait(dd, ACMD41);

			if(s == 0) {
				dd->_card_info.type = MMC_CARD_V1;
				r = RV_OK;
			} else {

				s = mmc_cmd_and_wait(dd, CMD1);

				if(s == 1) {
					dd->_card_info.type = MMC_CARD_V3;
					r = RV_OK;
				} else {
					r = RV_EIO;
					goto out;
				}
			}
		}
	}

out:
	if(r == RV_OK) {
		mmc_set_status(dev, MMC_STATUS_READY, RV_OK);

		if(dd->_card_info.type == MMC_CARD_V1 ||
		   dd->_card_info.type == MMC_CARD_V2 ||
		   dd->_card_info.type == MMC_CARD_V3) {
			s = mmc_cmd(dd, CMD16, 512);
		}


		struct mmc_card_info *info = &dd->_card_info;

		info->block_size = 512;

		uint8_t csd[16];
		s = mmc_cmd_data(dd, CMD9, 0, csd, sizeof(csd));
		
		uint32_t val;
		
		if ((csd[0] >> 6) == 1) {      

			/* SDC ver 2.00 */

			val = csd[9] + ((WORD)csd[8] << 8) + ((DWORD)(csd[7] & 63) << 16) + 1;
			info->sector_count = val << 10;
		} else {                               

			/* SDC ver 1.XX or MMC ver 3 */

			uint8_t n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
			val = (csd[8] >> 6) + ((WORD)csd[7] << 2) + ((WORD)(csd[6] & 3) << 10) + 1;
			info->sector_count = val << (n-9);
		}
	}

	if(r != RV_OK && r != RV_EBUSY) {
		mmc_set_status(dev, MMC_STATUS_ERROR, r);
		dd->_card_info.type = MMC_CARD_UNKNOWN;
	}
}


static void mmc_poll(struct dev_descriptor *desc, void *ptr)
{
	struct dev_mmc *dev = desc->dev;

	if(dev->drv == &drv_mmc_spi) {

		struct drv_mmc_spi_data *dd = dev->drv_data;

		uint8_t v;
		gpio_get_pin(dd->dev_gpio_detect, dd->detect_gpio_pin, &v);

		if(dd->_status == MMC_STATUS_IDLE || dd->_status == MMC_STATUS_ERROR) {
			mmc_detect(dev);
		}

		if(dd->_status == MMC_STATUS_READY) {
			/* todo: add alive check */
		}
	}
}


void on_ev_tick_1hz(event_t *ev, void *data)
{
	dev_foreach(DEV_TYPE_MMC, mmc_poll, NULL);
}

EVQ_REGISTER(EV_TICK_1HZ, on_ev_tick_1hz);


static rv readh(struct dev_mmc *dev, mmc_addr_t addr, void *buf, size_t len)
{
	struct drv_mmc_spi_data *dd = dev->drv_data;

	rv r = RV_ENOTREADY;

	if(dd->_status == MMC_STATUS_READY) {

		if(dd->_card_info.type == MMC_CARD_V2_BLOCK) {
			//addr *= 512;
		}

		uint8_t s;

		s = mmc_cmd_data(dd, CMD17, addr, buf, len);

		if(s == 1) r = RV_OK;
		cs(dd, 1);
	}

	if(r != RV_OK) {
		mmc_set_status(dev, MMC_STATUS_ERROR, RV_EIO);
	}

	return r;
}


static rv writeh(struct dev_mmc *dev, mmc_addr_t addr, const void *buf, size_t len)
{
	struct drv_mmc_spi_data *dd = dev->drv_data;
	
	rv r = RV_ENOTREADY;

	if(dd->_status == MMC_STATUS_READY) {

		if(dd->_card_info.type == MMC_CARD_V2_BLOCK) {
			//addr *= 512;
		}

		uint8_t s;

		s = mmc_cmd_data_write(dd, CMD24, addr, buf, len);

		if(s == 1) r = RV_OK;
		cs(dd, 1);
	}

	if(r != RV_OK) {
		mmc_set_status(dev, MMC_STATUS_ERROR, RV_EIO);
	}

	return r;
}


static rv get_card_info(struct dev_mmc *dev, struct mmc_card_info *info)
{
	struct drv_mmc_spi_data *dd = dev->drv_data;
	memcpy(info, &dd->_card_info, sizeof(*info));
	return RV_OK;
}


const struct drv_mmc drv_mmc_spi = {
	.init = init,
	.read = readh,
	.write = writeh,
	.get_card_info = get_card_info,
};

/*
 * End
 */

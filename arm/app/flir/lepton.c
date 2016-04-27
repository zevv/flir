
#include <string.h>

#include "bios/bios.h"
#include "bios/i2c.h"
#include "bios/arch.h"
#include "bios/button.h"
#include "bios/uart.h"
#include "bios/printd.h"
#include "bios/gpio.h"
#include "bios/cmd.h"
#include "bios/evq.h"
#include "bios/led.h"
#include "bios/can.h"
#include "bios/spi.h"

#include "lib/config.h"
#include "lib/atoi.h"

#include "arch/lpc/inc/chip.h"

#include "lepton.h"

uint8_t img[60][80];


static void lepton_read_packet(void)
{
	uint16_t buf[82];

	spi_read(&spi0, buf, sizeof(buf));

	uint16_t id = buf[0] & 0x0fff;

	if(id < 60) {

		uint16_t *pi = (void *)buf + 4;
		uint8_t *po = img[id];

		uint8_t x;
		uint16_t v = 0;
		uint16_t w = 0;

		for(x=0; x<63; x++) {

			v = *pi++;

			if(x == 0) {
				w = v;
				*po++ = (w & 0xff);
				*po++ = (w >> 8);
			} else {
				int32_t d = v - w;
				if(d < -127) d = -127;
				if(d > 127) d = 127;
				w += d;
				*po++ = d;
			}

		}
	}
}


static void lepton_read_frame(void)
{
	uint8_t i;

	for(i=0; i<80; i++) {
		lepton_read_packet();
	}
}


static void on_ev_tick(event_t *ev, void *data)
{
	static uint8_t n = 0;
	
	if(++n == 3) {
		lepton_read_frame();
		n = 0;
	}
}

EVQ_REGISTER(EV_TICK_100HZ, on_ev_tick);


void lepton_write(enum lepton_mod mod, uint16_t reg, uint16_t val)
{
	
}


void lepton_run(enum lepton_mod mod, uint8_t cmd_id)
{
	uint8_t cmd[] = { 0x00, 0x04, mod, LEPTON_CMD_TYPE_RUN | cmd_id };
	i2c_xfer(&i2c0, 0x2A, cmd, sizeof(cmd), NULL, 0);
}


void lepton_get(enum lepton_mod mod, uint8_t cmd_id, void *data, size_t len)
{
	uint8_t cmd[4] = { 0x00, 0x04, mod, LEPTON_CMD_TYPE_GET | cmd_id };
	i2c_xfer(&i2c0, 0x2A, cmd, sizeof(cmd), data, len);
}


static void lepton_set(enum lepton_mod mod, uint8_t cmd_id, void *data, size_t len)
{
	/* data registers */

	uint8_t buf_data[32] = { 0x00, 0x08 };
	memcpy(buf_data+2, data, len);
	i2c_xfer(&i2c0, 0x2A, buf_data, len + 2, NULL, 0);

	/* data len */

	uint8_t buf_len[4] = { 0x00, 0x06, (len >> 8), len & 0xff };
	i2c_xfer(&i2c0, 0x2A, buf_len, 4, NULL, 0);

	/* cmd */

	uint8_t buf_cmd[4] = { 0x00, 0x04, mod, LEPTON_CMD_TYPE_SET | cmd_id };
	i2c_xfer(&i2c0, 0x2A, buf_cmd, 4, NULL, 0);
}


void lepton_set_32(enum lepton_mod mod, uint8_t cmd_id, uint32_t val)
{
	uint8_t data[4] = {
		(val >> 24) & 0xff,
		(val >> 16) & 0xff,
		(val >>  8) & 0xff,
		(val >>  0) & 0xff,
	};
	lepton_set(mod, cmd_id, data, sizeof(data));
}
			
void lepton_set_16(enum lepton_mod mod, uint8_t cmd_id, uint16_t val)
{
	uint8_t data[2] = {
		(val >>  8) & 0xff,
		(val >>  0) & 0xff,
	};
	lepton_set(mod, cmd_id, data, sizeof(data));
}


static rv on_cmd_lepton(uint8_t argc, char **argv)
{
	rv r = RV_EINVAL;

	if(argc >= 1u) {
		char cmd = argv[0][0];

		if(cmd == 'a' && argc >= 2u) {
			lepton_set_16(LEPTON_MOD_AGC, LEPTON_CMD_ID_AGC, a_to_s32(argv[1]));
			r = RV_OK;
		}
		
		if(cmd == 't' && argc >= 2u) {
			lepton_set_16(LEPTON_MOD_SYS, LEPTON_CMD_ID_TELEMETRY, a_to_s32(argv[1]));
			r = RV_OK;
		}
	}

	return r;
}

CMD_REGISTER(lepton, on_cmd_lepton, "a[gc] <0|1>");


/*
 * End
  */

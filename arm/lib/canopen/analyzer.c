
#include <string.h>

#include "bios/bios.h"
#include "bios/arch.h"
#include "bios/eeprom.h"
#include "bios/printd.h"
#include "bios/log.h"
#include "bios/evq.h"
#include "bios/cmd.h"
#include "bios/led.h"
#include "bios/can.h"

#include "lib/canopen/analyzer.h"
#include "lib/canopen/inc/canopen.h"
#include "lib/canopen/hal-can.h"

static uint8_t enable_dump = 0;


static void decode_canopen(can_addr_t addr, const uint8_t *data, size_t len)
{
	uint16_t fncode =  (addr & 0x780u);
	uint8_t node_id = (addr & 0x07fu);
	char line[80];
	uint8_t pos = 0;

	rv aux(uint8_t c)
	{
		if(pos < sizeof(line)-1) {
			line[pos] = (char)c;
			pos ++;
		}
		return RV_OK;
	}

	fprintd(aux, " %3d:", node_id);
		
	uint16_t idx = 0, sidx = 0;
	uint32_t val = 0;
	uint8_t cs = 0;

	if(len >= 1) {
		cs = (data[0] >> 5);
	}

	if(len == 8) {
		idx = data[1] + (data[2] << 8);
		sidx = data[3];
		val = data[4] + (data[5] << 8) + (data[6] << 16) + (data[7] << 24);
	}

	if(fncode == 0x600 && len == 8) {
		switch(cs) {
			case 0: fprintd(aux, " SDO dl seg req"); break;
			case 1: fprintd(aux, " SDO dl req %04x.%d = %d", idx, sidx, val); break;
			case 2: fprintd(aux, " SDO ul req %04x.%d", idx, sidx); break;
			case 3: fprintd(aux, " SDO ul seg req"); break;
			case 4: fprintd(aux, " SDO ab     %04x.%d %s", idx, sidx, co_strerror(val)); break;
		}
	}

	else if(fncode == 0x580 && len > 4) {
		uint8_t n = 7 - ((data[0] >> 1) & 0x7);
		uint8_t t = !!(data[0] & 0x10);
		uint8_t c = !!(data[0] & 0x01);
		switch(cs) {
			case 0: fprintd(aux, " SDO ul seg rsp, n=%d t=%d c=%d", n, t, c); break;
			case 1: fprintd(aux, " SDO dl seg rsp, n=%d t=%d c=%d", n, t, c); break;
			case 2: fprintd(aux, " SDO ul rsp %04x.%d = %d", idx, sidx, val); break;
			case 3: fprintd(aux, " SDO dl rsp %04x.%d = %d", idx, sidx, val); break;
			case 4: fprintd(aux, " SDO ab     %04x.%d %s", idx, sidx, co_strerror(val)); break;
		}
	}
	
	else if(fncode == 0x000 && len == 2) {
		fprintd(aux, " NMT ");
		uint8_t cmd = data[0];
		uint8_t dest_id = data[1];
		fprintd(aux, " to");
		if(dest_id == 0) {
			fprintd(aux, " all");
		} else {
			fprintd(aux, " %d", dest_id);
		}
		switch(cmd) {
			case 0x01: fprintd(aux, " StartRemoteNode"); break;
			case 0x02: fprintd(aux, " StopRemoteNode"); break;
			case 0x80: fprintd(aux, " EnterPreOperationalState"); break;
			case 0x81: fprintd(aux, " ResetNode"); break;
			case 0x82: fprintd(aux, " ResetCommunication"); break;
			default:   fprintd(aux, " ?"); break;
		}
	}

	else if(fncode == 0x700 && len == 1) {
		fprintd(aux, " NMT ");
		switch(data[0]) {
			case   0: fprintd(aux, " Initialising"); break;
			case   1: fprintd(aux, " Disconnected"); break;
			case   2: fprintd(aux, " Connecting"); break;
			case   3: fprintd(aux, " Preparing"); break;
			case   4: fprintd(aux, " Stopped"); break;
			case   5: fprintd(aux, " Operational"); break;
			case 127: fprintd(aux, " PreOperational"); break;
			default:  fprintd(aux, " ?"); break;
		}
	}

	line[pos] = '\0';
	logd(LG_DBG, "%s", line);
}


void canalyzer_dump(const char *prefix, struct dev_can *dev, enum can_address_mode_t fmt, can_addr_t addr, const uint8_t *data, size_t len)
{
	if(enable_dump) {
		printd("%s ", prefix);

		if(fmt == CAN_ADDRESS_MODE_EXT) {
			printd("%08x ", addr);
		} else {
			printd("     %03x ", addr);
		}

		uint8_t i;
		for(i=0; i<8; i++) {
			if(i < len) {
				printd(" %02x", data[i]);
			} else {
				printd(" ..");
			}
		}

		decode_canopen(addr, data, len);
	}
}


static rv on_cmd_canalyze(uint8_t argc, char **argv)
{
	rv r = RV_EINVAL;

	if(argc >= 1u) {
		char cmd = argv[0][0];

		if(cmd == '0') {
			enable_dump = 0;
			r = RV_OK;
		}

		if(cmd == '1') {
			enable_dump = 1;
			r = RV_OK;
		}

		if(cmd == 'f') {
			decode_canopen(0x581, (uint8_t *)"\x80\xf8\x2f\x05\x24\x00\x00\x08", 8);
			r = RV_OK;
		}

	}

	return r;
}

CMD_REGISTER(canalyze, on_cmd_canalyze, "[1|0]");

/*
 * End
 */

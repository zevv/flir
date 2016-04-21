
/*
 * http://www.nmea.org/Assets/2000-explained-white-paper.pdf
 *
 * NMEA 2000 uses the 8 'data' bytes as follows:
 *
 * data[0] is an 'order' that increments, or not (depending a bit on
 * implementation).  If the size of the packet <= 7 then the data follows in
 * data[1..7] If the size of the packet > 7 then the next byte data[1] is the
 * size of the payload and data[0] is divided into 5 bits index into the fast
 * packet, and 3 bits 'order that increases.
 *
 * This means that for 'fast packets' the first bucket (sub-packet) contains 6
 * payload bytes and 7 for remaining. Since the max index is 31, the maximal
 * payload is 6 + 31 * 7 = 223 bytes
 */

#include <limits.h>
#include <stdint.h>
#include <string.h>

#include "bios/bios.h"
#include "bios/printd.h"
#include "bios/arch.h"

#include "lib/config.h"

#include "app/bci/bci-api.h"
#include "app/bci/modules.h"


#define NMEA_MANUFACTURER_CODE_SUPER_B           0x123
#define NMEA_MANUFACTURER_CODE_VICTRON           0x166
#define NMEA_DEVICE_FUNCTION_BATTERY               170
#define NMEA_DEVICE_CLASS_ELECTRICAL_GENERATION	    35
#define NMEA_INDUSTRY_GROUP_MARINE                   4

#define PGN_SUPERB_ERROR_STATUS    65280 /* 0x0ff00 */
#define PGN_ISO_ADDRESS_CLAIM      60928 /* 0x0ee00 */
#define PGN_DC_DETAILED_STATUS    127506 /* 0x1f212 */
#define PGN_BATTERY_STATUS        127508 /* 0x1f214 */
#define PGN_PRODUCT_INFORMATION   126996 /* 0x1f014 */

static const char hex[] = "0123456789ABCDEF";
static volatile uint16_t ticks = 0;

struct pgn_060928 {
	unsigned unique_id: 21;
	unsigned manufacturer_code: 11;
	unsigned device_instance: 8;
	unsigned device_function: 8;
	unsigned reserved1: 1;
	unsigned device_class: 7;
	unsigned system_instance: 4;
	unsigned industry_group: 3;
	unsigned reserved2: 1;
} __attribute__ (( __packed__ ));


struct pgn_127506 {
	uint8_t sid;
	uint8_t dc_instance;
	uint8_t dc_type;
	uint8_t SOC;			 /* % */
	uint8_t SOH;			 /* % */
	uint16_t time_remaining; /* Minutes */
	uint16_t U_ripple;       /* V */
} __attribute__ (( __packed__ ));


struct pgn_127508 {
	uint8_t battery_id;
	uint16_t U;              /* 0.01 V */
	uint16_t I;              /* 0.10 A */
	uint16_t T;              /* 0.01 K */
	uint8_t sid;
} __attribute__ (( __packed__ ));


struct pgn_126996 {
	uint16_t nmea2000_version;
	uint16_t product_code;
	char model_id[32];
	char software_version[32];
	char model_version[32];
	char model_serial_code[32];
	uint8_t certification_level;
	uint8_t load_equivalency;
} __attribute__ (( __packed__ ));


struct pgn_59904 {
	unsigned pgn: 24;
} __attribute__ (( __packed__ ));


struct pgn_65280 {
	uint16_t errors;
	uint16_t last_errors;
	unsigned charge_ok: 1;
	unsigned discharge_ok: 1;
} __attribute__ (( __packed__ ));


struct eedata {
	uint8_t address;
} __attribute__ (( __packed__ ));


enum nsm_state {
	STATE_0_IDLE,
	STATE_1_WAIT_DELAY,
	STATE_2_TRANSMIT_ADDRESS_CLAIM,
	STATE_3_RX_CAN_FRAME_PROCESS,
	STATE_4_FETCH_NEXT_MY_ADDRESS,
	STATE_5_RX_CAN_FRAME_PROCESS,
	STATE_6_WAITING_FOR_DELAY,
	STATE_7_TRANSMIT_ADDRESS_CLAIM,
	STATE_8_RX_CAN_FRAME_PROCESS,
	STATE_9_TRANSMIT_ADDRESS_CLAIM,
};


struct nmea_state_machine {

	/* NMEA state machine variables */

	enum nsm_state state;
	int IdleState;
	volatile int DelayComplete;
	int IWIN;
	int ILOSE;
	int REQUEST_PGN;

	/* Helper variables */

	uint64_t name_mine;
	uint64_t name_recv;
	volatile int ticks;
	uint8_t address;
};



static void send_pgn(const struct module *mod, uint8_t prio, uint32_t pgn, void *data, int len);



static struct nmea_state_machine nsm;
static struct eedata eedata;

static struct api_can_addr can_filter[] = {
	{ 0x0018eeff00, 0x00ffffff00 },  /* ISO address claim */
	{ 0x0018ea0000, 0x00ffff0000 },  /* ISO request */
	{ 0x001cef0000, 0x00ffff0000 },  /* Manufacturer proprietary */
};



static void nsm_start_random_delay(const struct module *mod)
{
	module_printd(mod, "Random delay\n");
	nsm.ticks = 2; /* I choose this with a dice, so it must be random */
	nsm.DelayComplete = 0;
}


static void nsm_start_250ms_delay(const struct module *mod)
{
	module_printd(mod, "250msec delay\n");
	nsm.ticks = 3;
	nsm.DelayComplete = 0;
}


static void nsm_transmit_address_claim(const struct module *mod, uint8_t addr)
{
	module_printd(mod, "TX address claim for %d", addr);
	send_pgn(mod, 6, PGN_ISO_ADDRESS_CLAIM | 0xff, &nsm.name_mine, sizeof nsm.name_mine);
}


static void nsm_rx_can_frame_process(const struct module *mod)
{
	if(nsm.name_recv) {
		if(nsm.name_recv < nsm.name_mine) {
			nsm.ILOSE = 1;
		} else {
			nsm.IWIN = 1;
		}
		nsm.name_recv = 0;
	} else {
		nsm.ILOSE = 0;
		nsm.IWIN = 0;
	}
}


static void nsm_set_state(const struct module *mod, enum nsm_state newstate, const char *reason)
{
	module_printd(mod, "%d -> %d : %s", nsm.state, newstate, reason);
	nsm.state = newstate;
}


static void got_address(const struct module *mod, uint8_t address)
{
	module_printd(mod, "Claimed address %d", address);
	if(address != eedata.address) {
		eedata.address = address;
		config_write("nmea2000", &eedata, sizeof(eedata));
	}
}


/*
 * Handle the NMEA address claim state machine
 */

static void nsm_poll(const struct module *mod)
{

	switch(nsm.state) {

		case STATE_0_IDLE:
			if(!nsm.IdleState) {
				nsm_start_random_delay(mod);
				nsm_set_state(mod, STATE_1_WAIT_DELAY, "End of idle state\n");
			}
			break;

		case STATE_1_WAIT_DELAY:
			if(nsm.DelayComplete) {
				nsm_set_state(mod, STATE_2_TRANSMIT_ADDRESS_CLAIM, "Delay complete\n");
			}
			break;

		case STATE_2_TRANSMIT_ADDRESS_CLAIM:
			nsm_transmit_address_claim(mod, nsm.address);
			nsm_start_250ms_delay(mod);
			nsm_set_state(mod, STATE_3_RX_CAN_FRAME_PROCESS, "Can msg sent\n");
			break;

		case STATE_3_RX_CAN_FRAME_PROCESS:
			nsm_rx_can_frame_process(mod);
			if(nsm.ILOSE) {
				nsm_set_state(mod, STATE_4_FETCH_NEXT_MY_ADDRESS, "I lose\n");
				break;
			}
			if(nsm.IWIN) {
				nsm_set_state(mod, STATE_2_TRANSMIT_ADDRESS_CLAIM, "I win\n");
				break;
			}
			if(nsm.DelayComplete) {
				nsm_set_state(mod, STATE_5_RX_CAN_FRAME_PROCESS, "Delay complete\n");
				got_address(mod, nsm.address);
				break;
			}
			break;

		case STATE_4_FETCH_NEXT_MY_ADDRESS:
			if(nsm.address < 252) {
				nsm.address = nsm.address + 1;
				nsm_set_state(mod, STATE_2_TRANSMIT_ADDRESS_CLAIM, "Try new address\n");
			} else {
				nsm_start_random_delay(mod);
				nsm_set_state(mod, STATE_6_WAITING_FOR_DELAY, "No more addresses\n");
			}
			break;

		case STATE_5_RX_CAN_FRAME_PROCESS:
			nsm_rx_can_frame_process(mod);
			if(nsm.ILOSE) {
				nsm_set_state(mod, STATE_4_FETCH_NEXT_MY_ADDRESS, "I lose\n");
				break;
			}
			if(nsm.IWIN) {
				nsm_set_state(mod, STATE_9_TRANSMIT_ADDRESS_CLAIM, "I win\n");
				break;
			}
			if(nsm.REQUEST_PGN) {
				nsm.REQUEST_PGN = 0;
				nsm_set_state(mod, STATE_9_TRANSMIT_ADDRESS_CLAIM, "Request PGN\n");
				break;
			}
			break;

		case STATE_6_WAITING_FOR_DELAY:
			if(nsm.DelayComplete) {
				nsm_set_state(mod, STATE_7_TRANSMIT_ADDRESS_CLAIM, "Delay complete\n");
				break;
			}
			break;

		case STATE_7_TRANSMIT_ADDRESS_CLAIM:
			nsm_transmit_address_claim(mod, 254);
			nsm_set_state(mod, STATE_8_RX_CAN_FRAME_PROCESS, "Can msg sent\n");
			break;

		case STATE_8_RX_CAN_FRAME_PROCESS:
			if(nsm.REQUEST_PGN) {
				nsm.REQUEST_PGN = 0;
				nsm_start_random_delay(mod);
				nsm_set_state(mod, STATE_6_WAITING_FOR_DELAY, "Request PGN\n");
				break;
			}
			break;

		case STATE_9_TRANSMIT_ADDRESS_CLAIM:
			nsm_transmit_address_claim(mod, nsm.address);
			nsm_set_state(mod, STATE_5_RX_CAN_FRAME_PROCESS, "Can msg sent, ready\n");
			got_address(mod, nsm.address);
			break;

		default:
			break;
	}
}


/*
 * Transmit PGN data. Send as plain data when <= 8 bytes, or encode using 'fast
 * packet' if more.
 */

static void send_pgn(const struct module *mod, uint8_t prio, uint32_t pgn, void *data, int len)
{
	uint8_t seq = 0;
	uint8_t buf[8];
	uint32_t can_id = (prio << 26) | (pgn << 8) | nsm.address;
	static uint8_t order = 0;

	if(len <= 8) {
		can_tx(&can0, CAN_ADDRESS_MODE_EXT, can_id, data, len);
	} else {

		buf[0] = (seq++) | ((order & 0x07) << 5);
		buf[1] = len;
		memcpy(buf+2, data, 6);
		can_tx(&can0, CAN_ADDRESS_MODE_EXT, can_id, buf, sizeof buf);
		data += 6;
		len -= 6;

		while(len != 0) {
			buf[0] = (seq++) | ((order & 0x07) << 5);
			int n = len;
			if(n > 7) n = 7;
			memcpy(buf+1, data, n);
			memset(buf+1+n, 0xff, 7-n);
			can_tx(&can0, CAN_ADDRESS_MODE_EXT, can_id, buf, sizeof buf);
			data += n;
			len -= n;
		}

		order ++;
	}
}



/*
 * Calculate time-to-empty for the battery, depending on the current SOC and
 * discharge current. Some heuristics are in place to avoid jumpy ETA's.
 */

static uint32_t calc_eta(struct accum_batt_data *ac)
{
	static int32_t eta_disp = 0;
	static int32_t n = 0;

	int32_t soc = ac->soc;
	int64_t Q_soc = ac->Q_design * soc / 100;
	int32_t I = ac->I; /* mA */
	int32_t eta_est = 0;

	if(I < 0) {
		eta_est = Q_soc / -I;
	}

	if(eta_disp * 10 > eta_est * 11 || eta_disp * 11 < eta_est * 10) {
		eta_disp = eta_est;
	}

	if(eta_disp > 0 && n%2 == 0) eta_disp --;
	if(eta_disp > 0 && eta_disp > eta_est) eta_disp --;
	n++;

	return eta_disp / 60;
}


static rv fn_init(const struct module *mod)
{
	module_printd(mod, "NMEA-2000 init\n");

	can_set_speed(&can0, CAN_SPEED_250_KB);

	/* Set error inhibit time so the oceanvolt generator has time to clear the
	 * error before the contactor is switched off */

	//api_switch_error_inhibit(mod, 10);

	/* Fill in my NMEA NAME */

	struct pgn_060928 name;

	name.unique_id = arch_get_unique_id();
	name.manufacturer_code = NMEA_MANUFACTURER_CODE_VICTRON;
	name.device_instance = 0;
	name.device_function = NMEA_DEVICE_FUNCTION_BATTERY;
	name.reserved1 = 0;
	name.device_class = NMEA_DEVICE_CLASS_ELECTRICAL_GENERATION;
	name.system_instance = 0;
	name.industry_group = NMEA_INDUSTRY_GROUP_MARINE;
	name.reserved2 = 1;

	/* Create EEPROM allocation */

	config_read("nmea2000", &eedata, sizeof eedata);

	/* Initialize state machine */

	nsm.IdleState = 0;
	nsm.DelayComplete = 0;
	nsm.ticks = 0;
	memcpy(&nsm.name_mine, &name, sizeof name);
	nsm_set_state(mod, STATE_0_IDLE, "Init\n");
	nsm.address = eedata.address;
	if(nsm.address >= 252) nsm.address = 1;

	return RV_OK;
}


/* 
 * PGN 127506: DC Detailed Status 
 */

static void send_127506(const struct module *mod, uint8_t sid)
{
	struct accum_batt_data ac;
	bci_get_accum_data(&ac);

	int32_t soc = ac.soc;
	int32_t eta = calc_eta(&ac);

	struct pgn_127506 m;
	m.sid = sid;
	m.dc_instance = 0;
	m.dc_type = 0x00; /* 00 = battery */
	m.SOC = soc;
	m.SOH = 100;
	m.time_remaining = eta;
	m.U_ripple = 0;
	send_pgn(mod, 6, PGN_DC_DETAILED_STATUS, &m, sizeof m);

}


/* 
 * PGN 127508: Battery Status 
 */ 

static void send_127608(const struct module *mod, uint8_t sid)
{
	struct accum_batt_data ac;
	bci_get_accum_data(&ac);

	struct pgn_127508 m;
	m.battery_id = 0;
	m.U = ac.U * 100;
	m.I = ac.I * 10;
	m.T = (ac.T_batt + 273.15) * 100;
	m.sid = sid;
	send_pgn(mod, 6, PGN_BATTERY_STATUS, &m, sizeof m);
}

/*
 * PGN 65280: super-b proprietary error status bits
 */

static void send_65280(const struct module *mod, uint8_t sid)
{
	struct pgn_65280 m;
	struct accum_batt_data ac;
	bci_get_accum_data(&ac);

	m.errors = ac.error_flags;
	m.last_errors = ac.last_error_flags;
	m.charge_ok = m.discharge_ok = (m.errors == 0);

	send_pgn(mod, 6, PGN_SUPERB_ERROR_STATUS, &m, sizeof m);
}


/* PGN pgn_126996: Product information */

static void send_126996(const struct module *mod)
{
	module_printd(mod, "send PGN 126996\n");

	struct pgn_126996 m;
	memset(&m, 0xff, sizeof(m));

	m.nmea2000_version = 1301;
	m.product_code = 1963;
	memcpy(&m.model_id, "BMV", 3);
	memcpy(&m.software_version, "1.20", 4);
	memcpy(&m.model_version, "1.0", 3);
	memcpy(&m.model_serial_code, "0026651", 7);
	m.certification_level = 1;
	m.load_equivalency = 1;

	send_pgn(mod, 6, PGN_PRODUCT_INFORMATION, &m, sizeof m);
}



static rv fn_timer(const struct module *mod)
{
	static uint8_t sid = 0;
	static uint16_t prev_errors = 0;

	ticks++;

	if(nsm.DelayComplete == 0 && nsm.ticks > 0) {
		nsm.ticks --;
		if(nsm.ticks == 0) {
			nsm.DelayComplete = 1;
		}
	}

	struct accum_batt_data ac;
	bci_get_accum_data(&ac);

	if(ticks >= 15 || ac.error_flags != prev_errors) {

		sid = (sid + 1);
		if(sid > 250) sid = 0;

		send_127506(mod, sid);
		send_127608(mod, sid);
		send_65280(mod, sid);

		prev_errors = ac.error_flags;
		ticks = 0;
	}

	nsm_poll(mod);

	return RV_OK;
}


static void handle_proprietary(const struct module *mod, const uint8_t *data, int len)
{

	int i;

	if(memcmp(data+3, "\x00\x00\x01\x00\xFF", 5) == 0) {
		module_printd(mod, "rx Manufacturer proprietary 1\n");
		char *m[] = { 
			"\x66\x99\x00\x01\x00\x21\xA1\xFF",
			"\x66\x99\x10\x01\x04\x01\x00\xFE",
			"\x66\x99\x03\x01\x00\x00\x00\x00",
			"\x66\x99\x02\x01\x00\x20\x01\x00",
			"\x66\x99\x00\x01\x01\xFF\xFF\xFE",
			"\x66\x99\x02\x01\x01\xFF\xFF\xFF",
			"\x66\x99\x20\x01\x00\x00\x00\x00",
		};

		for(i=0; i<7; i++) {
			send_pgn(mod, 7, 0x0efff, m[i], 8);
		}
		return;
	}

	if(memcmp(data+3, "\x00\x02\x01\xFF\xFF", 5) == 0) {
		module_printd(mod, "rx Manufacturer proprietary 2\n");
		send_pgn(mod, 7, 0x0efff, "\x66\x99\x00\x03\xFF\xFF\xFF\x7F", 8);
		return;
	}

	if(memcmp(data+3, "\x00\x00\x01\xFF\xFF", 5) == 0) {
		module_printd(mod, "rx Manufacturer proprietary 3\n");
		send_pgn(mod, 7, 0x0efff, "\x66\x99\x01\x03\xFF\xFF\xFF\x7F", 8);
		return;
	}

	module_printd(mod, "rx Manufacturer proprietary unhandled\n");
}


static rv fn_recv(const struct module *mod, enum can_address_mode_t fmt, uint32_t can_id, void *data, size_t len)
{
	uint8_t src = (can_id & 0x000000ff) >> 0;
	uint8_t dst = (can_id & 0x0000ff00) >> 8;

	if((can_id & 0xffffff00) == 0x18eeff00) {
		module_printd(mod, "rx ISO address claim %d -> %d", src, dst);
		if(src == nsm.address) {
			memcpy(&nsm.name_recv, data, len);
		}
	}

	if((can_id & 0xffff0000) == 0x18ea0000) {
		struct pgn_59904 *m = data;
		module_printd(mod, "rx ISO request %d -> %d for PGN %d", src, dst, m->pgn);
		if(dst == nsm.address || dst == 255) {
			if(m->pgn == PGN_ISO_ADDRESS_CLAIM) nsm.REQUEST_PGN = 1;
			if(m->pgn == PGN_PRODUCT_INFORMATION) send_126996(mod);
		}
	}

	if((can_id & 0xffff0000) == 0x1cef0000) {
		handle_proprietary(mod, data, len);
	}

	/* todo: process requestpng or address claim messages */

	return RV_OK;
}


MOD_REGISTER(
	.id = 1,
	.name = "nmea2000",
	.description = "NMEA-2000",
	.revision = 100,
	.options = MOD_OP_USERSELECT,
	.can_filter = can_filter,
	.fn_init = fn_init,
	.fn_timer = fn_timer,
	.fn_recv = fn_recv,
)


/*
 * End
 */

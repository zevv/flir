
#include <string.h>

#include "bios/bios.h"
#include "bios/printd.h"
#include "bios/arch.h"
#include "bios/evq.h"
#include "bios/led.h"
#include "bios/cmd.h"
#include "bios/relay.h"
#include "bios/eeprom.h"
#include "bios/sysinfo.h"
#include "bios/log.h"
#include "bios/gpio.h"

#ifdef LIB_HTTPD
#include "lib/httpd/httpd.h"
static rv on_httpd_bci(struct http_client *c, struct http_request *req, void *fndata);
#endif

#include "lib/canopen/inc/canopen.h"
#include "lib/canopen/hal-can.h"
#include "lib/atoi.h"
#include "lib/config.h"

#include "app/bci/bci-api.h"

#include "app/bci/can-master.h"
#include "app/bci/can-scanner.h"

#include "app/bci/modules.h"


static CoHandle ch;

static void on_ev_boot(event_t *ev, void *data)
{
	(void)led_set(&led_green, LED_STATE_BLINK);
	(void)config_init(&eeprom0, 0, 4096);
	(void)sysinfo_init();

	struct sysinfo_version ver;
	sysinfo_get_version(&ver);
	char nam[SYSINFO_PROP_LEN];
	char loc[SYSINFO_PROP_LEN];
	sysinfo_get(SYSINFO_NAME, nam, sizeof(nam));
	sysinfo_get(SYSINFO_LOCATION, loc, sizeof(loc));
	logd(LG_INF, "BCI %s/%s v%d.%d (%d%s) %s %s", 
			nam, loc,
			ver.maj, ver.min, ver.rev, ver.dev ? "M" : "", 
			__DATE__, __TIME__);
       
	/* 
	 * Initialize CAN open library, the can master and the can scanner. The
	 * object dictionaries are defined in can-master.c
	 */

	coInitLibrary();
	co_bind_dev(0, &can1);
	ch = coInit(0, CANOPEN_CAN_BIT_RATE_250000, 1, ODapp, PDOmap);

	if(ch != NULL) {
		//coMasterBootDS302(ch);
		(void)can_master_init(ch);
		(void)can_scanner_init(ch);
	} else {
		logd(LG_WRN, "coInit() error");
	}

	modules_start();

#ifdef LIB_HTTPD
	struct httpd *h;
	rv r = httpd_init(8000, &h);
	if(r == RV_OK) {
		httpd_register(h, "/bci", on_httpd_bci, NULL);
	} else {
		printd("%s\n", rv_str(r));
	}
#endif

}

EVQ_REGISTER(EV_BOOT, on_ev_boot);


/*
 * Periodic timer to collect all battery data and calculate accumulated status
 */

static void on_ev_tick_1hz(event_t *ev, void *data)
{
	uint8_t i;
	uint8_t error = 0;

	for(i=1; i<127; i++) {
		enum batt_state state;
		(void)can_scanner_get_batt_state(i, &state);

		if(state == BATT_STATE_MISSING) {
			error = 1;
		}
	}

	if(error == 0) {
		led_set(&led_red, LED_STATE_OFF);
	} else {
		led_set(&led_red, LED_STATE_BLINK);
	}
}

EVQ_REGISTER(EV_TICK_1HZ, on_ev_tick_1hz);


static void on_ev_tick_100hz(event_t *ev, void *data)
{
	if(ch != NULL) {
		coPeriodicService(ch);
		coPoll();
	}
}

EVQ_REGISTER(EV_TICK_100HZ, on_ev_tick_100hz);


#ifdef LIB_HTTPD

static char *state_name(enum batt_state s)
{
	switch(s) {
		case BATT_STATE_UNKNOWN:      return "unknown";
		case BATT_STATE_SCANNING:     return "scanning";
		case BATT_STATE_TESTING:      return "testing";
		case BATT_STATE_AVAILABLE:    return "available";
		case BATT_STATE_UNAVAILABLE:  return "unavailable";
		case BATT_STATE_MISSING:      return "missing";
	}
	return "";
}


static rv on_httpd_bci(struct http_client *c, struct http_request *req, void *fndata)
{
	const char *cmd;
	rv r = RV_ENOENT;
	
	httpd_client_write(c, "Content-Type: application/json\r\n\r\n");
	httpd_client_write(c, "{ \"data\": { ");

	cmd = http_request_get_param(req, "cmd");

	if(cmd) {

		if(strcmp(cmd, "bci_full_scan") == 0) {
			const char *sfrom = http_request_get_param(req, "from");
			const char *sto = http_request_get_param(req, "to");
			uint8_t from = sfrom ? (uint8_t)atoi(sfrom) : 1;
			uint8_t to = sto ? (uint8_t)atoi(sto) : BATT_ID_MAX;
			r = bci_full_scan(from, to);
		}

		if(strcmp(cmd, "bci_get_batt_state") == 0) {
			enum batt_state state_list[BATT_ID_MAX+1];
			r = bci_get_batt_state(state_list, BATT_ID_MAX);
			if(r == RV_OK) {
				uint8_t i;
				for(i=0; i<=BATT_ID_MAX; i++) {
					httpd_client_printf(c, "\"%d\": \"%s\"", i, state_name(state_list[i]));
					if(i != BATT_ID_MAX) {
						httpd_client_write(c, ", ");
					}
				}
			}
		}

		if(strcmp(cmd, "bci_get_accum_data") == 0) {
			struct accum_batt_data data;
			r = bci_get_accum_data(&data);
			if(r == RV_OK) {
				httpd_client_printf(c, "\"error\": %d, ", data.error_flags);
				httpd_client_printf(c, "\"U\": %.3f, ", data.U);
				httpd_client_printf(c, "\"I\": %.3f, ", data.I);
				httpd_client_printf(c, "\"T_batt\": %.1f, ", data.T_batt);
				httpd_client_printf(c, "\"soc\": %d, ", data.soc);
				httpd_client_printf(c, "\"cycle_count\": %d ", data.cycle_count);
			};
		}
		
		if(strcmp(cmd, "batt_get_data") == 0) {

			const char *node_id = http_request_get_param(req, "node_id");
			if(node_id) {
				struct batt_data data;
				r = bci_get_batt_data(atoi(node_id), &data);
				if(r == RV_OK) {
					httpd_client_printf(c, "\"error\": %d, ", data.error);
					httpd_client_printf(c, "\"U\": %.3f, ", data.U);
					httpd_client_printf(c, "\"I\": %.3f, ", data.I);
					httpd_client_printf(c, "\"T_batt\": %.1f, ", data.T_batt);
					httpd_client_printf(c, "\"soc\": %d, ", data.soc);
					httpd_client_printf(c, "\"cycle_count\": %d ", data.cycle_count);
				};
			}
		}
		
		
	}

	if(r == RV_OK) {
		httpd_client_printf(c, "}, \"ok\": true }\n", r);
	} else {
		httpd_client_printf(c, "}, \"ok\": false, \"rv\": %d, \"err\": \"%s\" }\n", r, rv_str(r));
	}

	httpd_client_close(c);
	return RV_OK;
}

#endif

/*
 * Command line handler for 'bci' command
 */

static rv on_cmd_bci(uint8_t argc, char **argv)
{
	rv r = RV_EINVAL;

	if(argc >= 1u) {
		char cmd = argv[0][0];

		if(cmd == 's') {
			uint8_t id_from = 1;
			uint8_t id_to = 16;
			if(argc >= 2u) { id_from = (uint8_t)a_to_s32(argv[1]); }
			if(argc >= 3u) { id_to = (uint8_t)a_to_s32(argv[2]); }
			r = bci_full_scan(id_from, id_to);
		}

		if(cmd == 'p') {
			printd("pause\n");
			can_scanner_pause();
			r = RV_OK;
		}
		
		if(cmd == 'r') {
			printd("resume\n");
			can_scanner_resume();
			r = RV_OK;
		}

		if(cmd == 'l') {
			struct layout_info l;
			r = bci_get_layout(&l);
			if(r == RV_OK) {
				if(argc == 1) {
					printd("s:%d p:%d\n", l.n_series, l.n_parallel);
				} else if(argc == 3) {
					if(argv[1][0] == 's') {
						l.n_series = a_to_s32(argv[2]);
						r = bci_set_layout(&l);
					}
					if(argv[1][0] == 'p') {
						l.n_parallel = a_to_s32(argv[2]);
						r = bci_set_layout(&l);
					}
				} else {
					r = RV_EINVAL;
				}
			}
		}

	} else {


		uint8_t i;

		for(i=1; i<127; i++) {
			enum batt_state state;
			r = can_scanner_get_batt_state(i, &state);

			if((r == RV_OK) && 
					(state != BATT_STATE_UNAVAILABLE) &&
					(state != BATT_STATE_UNKNOWN)) {
				printd("%d: ", i);
				if(state == BATT_STATE_MISSING) { printd("missing"); }
				if(state == BATT_STATE_TESTING) { printd("testing"); }
				if(state == BATT_STATE_SCANNING) { printd("scanning"); }
				if(state == BATT_STATE_AVAILABLE) {
					struct batt_data data;
					(void)bci_get_batt_data(i, &data);
					printd("Q_design=%.0f U=%.3f I=%.3f soc=%d%%", 
							data.Q_design / 3600.0, data.U, data.I, data.soc);
				}
				printd("\n");
			}
		}

		r = RV_OK;
	}
	return r;
}

CMD_REGISTER(bci, on_cmd_bci, "s[can] [from_id] [to_id] | l[ayout] [s <N>|p <N>]");

/*
 * End
 */

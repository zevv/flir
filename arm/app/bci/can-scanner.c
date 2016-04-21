
/*
             +---------+
             | UNKNOWN |
             +---------+
                  |
                  | full scan
                  v
             +----------+
             | SCANNING |
             +----------+
              |        |       
    not found |        |  found
     in scan  |        | in scan
              v        v
  +-------------+   +---------+
  | UNAVAILABLE |   | TESTING |<--+
  +-------------+   +---------+   |
                      |           |
                      | poll      |
                      | ok        |
                      v           |
                +-----------+     |
                | AVAILABLE |     |
                +-----------+     |
                    |  |  ^       |
               poll |  |  | poll  |
               fail |  +--+ ok    |
                    v             | poll
                  +---------+     |  ok
                  | MISSING |-----+
                  +---------+     
               poll |     ^
               fail |     |
                    +-----+

 */


#include <string.h>

#include "bios/eeprom.h"
#include "bios/bios.h"
#include "bios/log.h"
#include "bios/evq.h"
#include "bios/led.h"
#include "bios/can.h"
#include "bios/arch.h"

#include "lib/config.h"
#include "lib/canopen/inc/canopen.h"
#include "lib/canopen/inc/canopenM.h"
#include "lib/canopen/hal-can.h"

#include "app/bci/bci-api.h"
#include "app/bci/can-scanner.h"
#include "app/bci/sdo-client.h"


struct batt {
	enum batt_state state;
	uint8_t node_id;

	struct batt_data data;

	struct {
		uint8_t busy;
		uint8_t item;
		uint32_t t_next;
	} poll;
};

struct poll_info {
	uint16_t objIdx;
	uint8_t subIdx;
};


struct can_scanner {
	enum can_scanner_state state;
	int pause;
	
	CoHandle ch;
	
	struct batt batt_list[BATT_ID_MAX+1u]; /* we index from 1, entry 0 is not used */

	struct {
		uint8_t node_id_first;
		uint8_t node_id_last;
		uint8_t node_id;
	} scan;
};


static void on_ev_tick_100hz(event_t *ev, void *data);
static void batt_set_state(struct batt *batt, enum batt_state state);

static struct can_scanner _scanner;
static struct can_scanner * const scanner = &_scanner;

static const struct poll_info poll_info_list[] = {
	{ 0x6000, 0 },
	{ 0x6010, 0 },
	{ 0x6020, 2 },
	{ 0x6020, 3 },
	{ 0x6060, 0 },
	{ 0x6081, 0 },
	{ 0x2001, 0 },
	{ 0x2010, 0 },
	{ 0x2011, 1 },
	{ 0x2011, 2 },
	{ 0x2011, 3 },
	{ 0x2011, 4 },
	{ 0x2014, 0 },
};

#define POLL_INFO_COUNT (sizeof(poll_info_list) / sizeof(poll_info_list[0]))



/*
 * Public functions
 */

rv can_scanner_init(CoHandle ch)
{
	scanner->ch = ch;
	scanner->state = CAN_SCANNER_STATE_POLL;

	uint32_t avail[4] = { 0 };
	config_read("batt_list", &avail, sizeof(avail));
	
	uint8_t i;
	for(i=BATT_ID_MIN; i<=BATT_ID_MAX; i++) {
		struct batt *batt = &scanner->batt_list[i];
		batt->node_id = i;

		if(avail[i/32] & (1<<(i%32))) {
			batt->state = BATT_STATE_TESTING;
		} else {
			batt->state = BATT_STATE_UNKNOWN;
		}
	}

	evq_register(EV_TICK_100HZ, on_ev_tick_100hz, NULL);

	return RV_OK;
}


rv can_scanner_get_state(enum can_scanner_state *state, uint8_t *perc_done)
{
	*state = scanner->state;

	uint32_t todo = scanner->scan.node_id_last - scanner->scan.node_id_first;
	if(todo > 0) {
		uint32_t done = scanner->scan.node_id - scanner->scan.node_id_first - 1;
		*perc_done = 100 * done / todo;
	}
	return RV_OK;
}


/*
 * Perform a full scan in the given range of node ids
 */

rv can_scanner_full_scan(uint8_t node_id_first, uint8_t node_id_last)
{
	uint8_t i;
	rv r = RV_OK;

	if(scanner->ch == NULL) {
		r = RV_EIO;
	}

	if(r == RV_OK) {
		if(scanner->state != CAN_SCANNER_STATE_POLL) {
			r = RV_EBUSY;
		}
	}

	if(r == RV_OK) {

		if((node_id_first >= BATT_ID_MIN) && (node_id_first <= BATT_ID_MAX) &&
		   (node_id_last  >= BATT_ID_MIN) && (node_id_last  <= BATT_ID_MAX) &&
		   (node_id_last > node_id_first)) {

			uint8_t my_node_id = coGetNodeId(scanner->ch);

			for(i=node_id_first; i<=node_id_last; i++) {

				struct batt *batt = &scanner->batt_list[i];

				if(i != my_node_id) {
					batt_set_state(batt, BATT_STATE_SCANNING);
				} else {
					batt_set_state(batt, BATT_STATE_UNAVAILABLE);
				}
			}

			scanner->state = CAN_SCANNER_STATE_SCAN;
			scanner->scan.node_id_first = node_id_first;
			scanner->scan.node_id_last = node_id_last;
			scanner->scan.node_id = node_id_first;

			logd(LG_INF, "scan start");

		} else {
			r = RV_EINVAL;
		}
	}

	return r;
}


rv can_scanner_get_batt_state(uint8_t node_id, enum batt_state *state)
{
	rv r;

	if((node_id >= 1u) && (node_id <= BATT_ID_MAX)) {
		*state = scanner->batt_list[node_id].state;
		r = RV_OK;
	} else {
		r = RV_EINVAL;
	}

	return r;
}


rv can_scanner_get_batt_data(uint8_t node_id, struct batt_data *data)
{
	rv r;

	if((node_id >= 1u) && (node_id <= BATT_ID_MAX)) {
		*data = scanner->batt_list[node_id].data;
		r = RV_OK;
	} else {
		r = RV_EINVAL;
	}

	return r;
}


rv can_scanner_pause(void)
{
	scanner->pause ++;
	return RV_OK;
}


rv can_scanner_resume(void)
{
	if(scanner->pause > 0) {
		scanner->pause --;
	}
	return RV_OK;
}


/***************************************************************************/

static void batt_set_state(struct batt *batt, enum batt_state state)
{
	enum batt_state sn = state;
	enum batt_state sp = batt->state;

	if((sp == BATT_STATE_AVAILABLE) && (sn == BATT_STATE_MISSING)) {
		logd(LG_WRN, "batt %d missing", batt->node_id);
	}

	if((sp == BATT_STATE_TESTING) && (sn == BATT_STATE_AVAILABLE)) {
		logd(LG_INF, "batt %d avail", batt->node_id);
	}

	batt->poll.item = 0;
	batt->state = state;
}


/*
 * Handle SDO response for batteries in AVAILABLE state. All properties are
 * converted from CANOpen units to SI units here
 */

static void handle_batt_sdo_response(struct batt *batt, uint16 oidx, uint8 sidx, uint8 *data, size_t len)
{
	uint32_t v = (uint32_t)data[0] | 
		     ((uint32_t)data[1] << 8) | 
		     ((uint32_t)data[2] << 16) | 
		     ((uint32_t)data[3] << 24);

	if(oidx == 0x1018u) {
		if(sidx == 3u) {
			batt->data.identity = v;
		}
	}

	if(oidx == 0x6010u) {
		batt->data.T_batt = (temperature)((int32_t)v) / 10.0;
	}
	
	if(oidx == 0x6020u) {
		if(sidx == 0x02u) {
			batt->data.Q_design = v * 3600;
		}
		if(sidx == 0x03u) {
			batt->data.I_charge_max = v;
		}
	}

	if(oidx == 0x6060u) {
		batt->data.U = (voltage)v / 1024.0;
	}
	
	if(oidx == 0x6081u) {
		batt->data.soc = (uint8_t)v;
	}

	if(oidx == 0x2010u) {
		batt->data.I = (current)((int32_t)v) / 1000.0;
	}

	if(oidx == 0x2011u) {
		if((sidx >= 1u) && (sidx <= 4u)) {
			batt->data.cell[sidx-1u].U = (voltage)v / 1000.0;
		}
	}

	if(oidx == 0x2014u) {
		batt->data.cycle_count = (uint16_t)v;
	}
}


static void on_batt_read(uint8_t node_id, uint16_t objidx, uint8_t subidx, void *data, size_t len, uint32_t error, void *fndata)
{
	struct batt *batt = fndata;

	batt->poll.busy = 0;

	switch(batt->state) {

		case BATT_STATE_SCANNING:
			if(data != NULL) {
				batt_set_state(batt, BATT_STATE_TESTING);
			} else {
				batt_set_state(batt, BATT_STATE_UNAVAILABLE);
			}
			break;

		case BATT_STATE_TESTING:
		case BATT_STATE_AVAILABLE:
			if(data != NULL) {
				handle_batt_sdo_response(batt, objidx, subidx, data, len);
			} else {
				logd(LG_WRN, "node %d err %04x.%d: %s", node_id, objidx, subidx, co_strerror(error));
				batt_set_state(batt, BATT_STATE_MISSING);
			}
			break;

		case BATT_STATE_MISSING:
			if(data != NULL) {
				batt_set_state(batt, BATT_STATE_TESTING);
			}
			break;

		case BATT_STATE_UNKNOWN:
		case BATT_STATE_UNAVAILABLE:
			logd(LG_WRN, "unexpected SDO response for node %d", node_id);
			break;
			
		default:
			/* misra */
			break;
	}

	/* On error, hold off further communication with this node for some time */

	if(error != 0u) {
		batt->poll.t_next = arch_get_ticks() + 500u;
	}

	batt->poll.busy = 0;
}


static rv on_tick_scanning(void)
{

	rv r = RV_OK;

	if(scanner->scan.node_id <= scanner->scan.node_id_last) {

		/* Find a free SDO and read device type from client */

		struct batt *batt = &scanner->batt_list[scanner->scan.node_id];

		if(batt->state == BATT_STATE_SCANNING) {
			r = sdo_read(scanner->ch, scanner->scan.node_id, CO_OBJDICT_DEVICE_TYPE, 0, on_batt_read, batt);
		}

		if(r == RV_OK) {
			scanner->scan.node_id ++;
		}

	} else {

		/* All scan read SDO's are sent out, wait for all the answers
		 * or timeouts */

		uint8_t i;

		uint8_t nbusy = 0;
		uint8_t nfound = 0;

		for(i=scanner->scan.node_id_first; i<=scanner->scan.node_id_last; i++) {
			struct batt *batt = &scanner->batt_list[i];
			if(batt->state == BATT_STATE_SCANNING) {
				nbusy ++;
			}
			if(batt->state == BATT_STATE_TESTING) {
				nfound ++;
			}
		}

		if(nbusy == 0u) {
			logd(LG_INF, "scan completed, %d batteries found", nfound);
			
			uint32_t avail[4] = { 0 };
			uint8_t i;
			for(i=BATT_ID_MIN; i<=BATT_ID_MAX; i++) {
				struct batt *batt = &scanner->batt_list[i];
				if(batt->state == BATT_STATE_AVAILABLE) {
					avail[i/32] |= (1<<(i%32));
				}
			}
			config_write("batt_list", &avail, sizeof(avail));
				
			scanner->state = CAN_SCANNER_STATE_POLL;
		}
	}

	return r;
}


/*
 * Poll one batt by sending SDO requests. The responses or errors are handled
 * asynchronously in handle_batt_sdo_response()
 */

static void poll_batt(struct batt *batt, uint8_t node_id)
{
	if((batt->state == BATT_STATE_AVAILABLE) || 
	   (batt->state == BATT_STATE_MISSING) ||
	   (batt->state == BATT_STATE_TESTING)) {
			
		const struct poll_info *pi = &poll_info_list[batt->poll.item];

		rv r = sdo_read(scanner->ch,  node_id, pi->objIdx, pi->subIdx, on_batt_read, batt);

		if(r == RV_OK) {

			batt->poll.busy = 1;
			batt->poll.item ++;

			if(batt->poll.item >= POLL_INFO_COUNT) {
				batt->poll.item = 0;
				batt->poll.t_next = arch_get_ticks() + 500u;

				if(batt->state == BATT_STATE_TESTING) {
					batt_set_state(batt, BATT_STATE_AVAILABLE);
				}
			}
		}
	}
}


static rv on_tick_poll(void)
{
	uint8_t i;
	uint32_t t_now = arch_get_ticks();

	for(i=1; i<=BATT_ID_MAX; i++) {
		struct batt *batt = &scanner->batt_list[i];
		if(t_now >= batt->poll.t_next) {
			if(batt->poll.busy == 0u) {
				poll_batt(batt, i);
			}
		}
	}

	return RV_OK;
}


static void on_ev_tick_100hz(event_t *ev, void *data)
{
	static uint8_t backoff = 0;

	if(backoff == 0) {
		if(!scanner->pause) {

			rv r;

			if(scanner->state == CAN_SCANNER_STATE_POLL) {
				r = on_tick_poll();
			}

			if(scanner->state == CAN_SCANNER_STATE_SCAN) {
				r = on_tick_scanning();
			}

			if(r != RV_OK) {
				backoff = 20;
			}
		}
	} else {
		backoff --;
	}
}


CoObjLength appGetObjectLen(CoHandle ch, uint16 objIdx, uint8 subIdx, coErrorCode *err)
{
	return 0;
}


/*
 * End
 */

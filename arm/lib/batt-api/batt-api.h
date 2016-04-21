
#ifndef lib_batt_api_h
#define lib_batt_api_h

#include "bios/bios.h"
#include "bios/printd.h"

#include "lib/canopen/inc/canopen.h"

#define BATT_API_BATT_COUNT 128

typedef uint8_t batt_id;

struct batt_essential {
	voltage U;
	current I;
	uint16_t error;
	uint8_t SOC;
	uint8_t online;
	uint8_t flags;
};

struct batt_request {
	current I;
	voltage U;
};

struct batt_temperature {
	temperature T_batt;
	temperature T_fuse;
	temperature T_shunt;
	temperature T_tab[3];
};


typedef void (*poll_cb)(rv r);

rv batt_api_init(CoHandle ch);
rv batt_api_add_poll_cb(batt_id id, poll_cb cb);

rv batt_api_full_scan(batt_id id_first, batt_id id_last);

rv batt_api_poll_essential(batt_id id, struct batt_essential *be);
	


#endif

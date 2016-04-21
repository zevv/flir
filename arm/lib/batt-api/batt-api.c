
#include "bios/bios.h"

#include "lib/canopen/inc/canopen.h"
#include "lib/canopen/hal-can.h"

#include "lib/batt-api/batt-api.h"
#include "lib/batt-api/sdo-client.h"

#define CB_COUNT 4

static CoHandle ch = NULL;



static struct poll_info *poll_info_by_id(batt_id id)
{
	struct poll_info *pi = NULL;

	if((id >= 1) && (id < BATT_API_BATT_COUNT)) {
		pi = &poll_info_list[id];
	}

	return pi;

}


rv batt_api_init(CoHandle _ch)
{
	ch = _ch;

	printd("size = %d\n", sizeof(poll_info_list));
	return RV_OK;
}


rv batt_api_add_poll_cb(batt_id id, poll_cb cb)
{
	struct poll_info *pi = poll_info_by_id(id);
	rv r;

	if(pi) {
		pi->cb_list[0] = cb;
		r = RV_OK;
	} else {
		r = RV_EINVAL;
	}

	return r;
}


rv batt_api_poll_essential(batt_id id, struct batt_essential *be)
{
	rv r;
	struct poll_info *pi = poll_info_by_id(id);

	if(pi) {
		pi->essential = be;
		r = RV_OK;
	} else {
		r = RV_EINVAL;
	}

	return r;
}


/*
 * End
 */


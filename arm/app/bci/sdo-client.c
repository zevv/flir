
#include <stdint.h>
#include <assert.h>

#include "bios/bios.h"
#include "bios/printd.h"

#include "lib/canopen/inc/canopen.h"
#include "lib/canopen/hal-can.h"

#include "app/bci/sdo-client.h"

struct sdo_client {
	uint8_t node_id;
	uint8_t sdo_no;
	uint16_t objidx;
	uint8_t subidx;
	sdo_read_callback fn;
	void *fndata;
};


struct sdo_client sdo_client_list[SDO_MAX_COUNT];


/*
 * Find a free client SDO and connect it to the node id
 */

static rv connect_sdo(CoHandle co, uint8_t node_id, uint8_t *sdo_no)
{
	uint8_t i = 0u;
	coErrorCode res;
	rv r = RV_EBUSY;

	for(i=0; i<SDO_MAX_COUNT; i++) {

		coStatusSDO s;
		
		s = coReadStatusSDO(co, i, SDO_CLIENT);

		if(s == SDO_READY) {

			uint16_t idx = CO_OBJDICT_SDO_CLIENT + i;

			res = coWriteObject(co, idx, 1, 0x600 + node_id);
			if(res == CO_OK) {
				res = coWriteObject(co, idx, 2, 0x580 + node_id);
			}
			if(res == CO_OK) {
				res = coWriteObject(co, idx, 3, node_id);
			}
			if(res == CO_OK) {
				*sdo_no = i; 
				r = RV_OK;
			} else {
				printd("Err %08x\n", res);
				r = RV_EIO;
			}
			break;
		}
	}

	return r;
}


rv sdo_read(CoHandle co, uint8_t node_id, uint16_t objidx, uint8_t subidx, sdo_read_callback fn, void *fndata)
{
	rv r = RV_EBUSY;
	uint8_t sdo_no;

	r = connect_sdo(co, node_id, &sdo_no);

	if(r == RV_OK) {

		assert(sdo_no < SDO_MAX_COUNT);
		struct sdo_client *sc = &sdo_client_list[sdo_no];
		assert(sc->fn == NULL);

		CoStackResult res;

		res = coReadObjectSDO(co, sdo_no, objidx, subidx);

		if(res == coRes_OK) {
			sc->node_id = node_id;
			sc->objidx = objidx;
			sc->subidx = subidx;
			sc->fn = fn;
			sc->fndata = fndata;
			r = RV_OK;
		} else {
			r = RV_EPROTO;
		}
	}

	return r;
}


coErrorCode appReadObjectResponseSDO(CoHandle ch, uint8 sdo_no, uint16 objIdx,
		uint8 subIdx, uint8 *bufPtr, CoObjLength pos, uint8 valid, CoSegTranAction flag)
{
	struct sdo_client *sc = &sdo_client_list[sdo_no];

	if(sc->fn) {
		sc->fn(sc->node_id, sc->objidx, sc->subidx, bufPtr, valid, 0, sc->fndata);
		sc->fn = NULL;
	}

	return 0;
}


void appTransferErrSDO(CoHandle ch, uint8 sdo_no, uint16 objIdx, uint8 subIdx, coErrorCode errCode)
{
	struct sdo_client *sc = &sdo_client_list[sdo_no];

	if(sc->fn) {
		sc->fn(sc->node_id, sc->objidx, sc->subidx, NULL, 0, errCode, sc->fndata);
		sc->fn = NULL;
	}
}


/*
 * End
 */


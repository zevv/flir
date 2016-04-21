
#include <string.h>
#include <stdint.h>

#include "plat.h"

#include "bios/arch.h"
#include "bios/bios.h"
#include "bios/printd.h"
#include "bios/evq.h"
#include "bios/led.h"
#include "bios/relay.h"
#include "bios/can.h"

#include "lib/canopen/inc/canopen.h"
#include "lib/canopen/inc/canopenM.h"
#include "lib/canopen/hal-can.h"


static CoHandle coh;

int32_t I_batt;
uint32_t U_batt;
uint32_t U_cell[4];
uint16_t T_batt;
uint8_t soc;

static coObjDictVarDesc
   ode_32v_r      = { 0,  NULL,   4, CO_OD_ATTR_READ | CO_OD_ATTR_PDO_MAPABLE | CO_OD_ATTR_VECTOR, 0 },
   ode_8_r        = { 0,  NULL,   1, CO_OD_ATTR_READ|CO_OD_ATTR_PDO_MAPABLE, 0},
   ode_16_r       = { 0,  NULL,   2, CO_OD_ATTR_READ|CO_OD_ATTR_PDO_MAPABLE, 0},
   ode_32_r       = { 0,  NULL,   4, CO_OD_ATTR_READ|CO_OD_ATTR_PDO_MAPABLE, 0};

const coPdoMapItem PDOmap[] = { 
        CO_PDO_MAP_END
};


static coObjDictEntry ODapp[] = {
	{ 0x2001,    0, &U_batt, &ode_32_r,  CO_ODDEF(CO_ODOBJ_VAR, CO_ODTYPE_INT32) },
	{ 0x2010,    0, &I_batt, &ode_32_r,  CO_ODDEF(CO_ODOBJ_VAR, CO_ODTYPE_INT32) },
	{ 0x2011,    4, &U_cell, &ode_32v_r, CO_ODDEF(CO_ODOBJ_VAR, CO_ODTYPE_INT32) },
	{ 0x6000,    0, &U_batt, &ode_32_r,  CO_ODDEF(CO_ODOBJ_VAR, CO_ODTYPE_INT32) },
	{ 0x6060,    0, &U_batt, &ode_32_r,  CO_ODDEF(CO_ODOBJ_VAR, CO_ODTYPE_INT32) },
	
	{ 0x2014,    0, &U_batt, &ode_32_r,  CO_ODDEF(CO_ODOBJ_VAR, CO_ODTYPE_INT32) },
	{ 0x6010,    0, &T_batt, &ode_16_r,  CO_ODDEF(CO_ODOBJ_VAR, CO_ODTYPE_INT16) },
	{ 0x6081,    0, &soc,    &ode_8_r,   CO_ODDEF(CO_ODOBJ_VAR, CO_ODTYPE_INT8) },
	{      0,    0, NULL,    NULL,         0}
};


static void on_ev_boot(event_t *ev, void *data)
{
	led_set(&led_yellow, LED_STATE_BLINK);

	printd("BMS starting\n");
        
	coInitLibrary();

	int can_id = 10;
	if(getenv("CAN_ID")) {
		can_id = atoi(getenv("CAN_ID"));
	}

	co_bind_dev(0, &can0);
	coh = coInit(0, CANOPEN_CAN_BIT_RATE_250000, can_id, ODapp, PDOmap);

	if(coh == NULL) {
		printd("coInit error\n");
		exit(0);
	}
}

EVQ_REGISTER(EV_BOOT, on_ev_boot);


static void on_ev_tick(event_t *ev, void *data)
{
	U_batt = (13.3 + (double)rand() / RAND_MAX) * 1024;
	U_cell[0] = U_cell[1] = U_cell[2] = U_cell[3] = 3340;
	T_batt = 21 * 10;
	I_batt = -1.5 * 1000;

	static double socf = 0;

	socf += 0.01;
	if(socf > 100) socf = 0;
	soc = socf;

	coPeriodicService(coh);
	coPoll();
}

EVQ_REGISTER(EV_TICK_100HZ, on_ev_tick);


bool appGetIdentityObject(CH uint32 *vendorId, uint32 *prodCode, uint32 *revNum, uint32 *serialNum)
{
	*vendorId = 0x0000037C;
	*prodCode = 0x00000002;
	*revNum   = (VERSION_DEV << 31) | (VERSION_MAJ << 24) | (VERSION_MIN << 16) | REVISION;
	*serialNum = 0xdef0;

	return true;
}


/*
 * End
 */


/*
 * Interfacing to Phillippi System Monitor
 *
 * The baud rate for PBUS is 250 Kbit/s
 * 
 * PSM sends a request for battery data
 * PSM sends to Super B the Message 0x04EFF70C without data bytes.
 * This means, PSM wants to receive battery data from Super B
 * Super B sends the requested battery data
 * After Super B has received the data request message 0x04EFF70C it answers with 2 Messages 
 * containing the battery data
 * 
 * 0x04EFF700
 *  D0 battery error status flags (L-byte)
 *  D1 battery error status flags (H-byte)
 *  D2 battery status flags
 *  D3 battery current (1. byte - lowest)
 *  D4 battery current (2. byte)
 *  D5 battery current (3. byte)
 *  D6 battery current (4. byte - highest)
 *  D7 Battery SOC
 * 
 * 0x04EFF701	
 *  D0 Cycle count  (L-byte)
 *  D1 Cycle count (H-byte)
 *  D2 battery temperature (L-byte )
 *  D3 battery temperature (H-byte)
 *  D4 battery voltage (1. byte - lowest)
 *  D5 battery voltage (2. byte)
 *  D6 battery voltage (3. byte)
 *  D7 battery voltage (4. byte – highest) 
 */

#include <limits.h>
#include <stdint.h>
#include <string.h>

#include "bios/bios.h"
#include "app/bci/bci-api.h"
#include "app/bci/modules.h"


#define PHILIPPI_ID_QUERY 0x04EFF70C
#define PHILIPPI_ID_RESP1 0x04EFF700
#define PHILIPPI_ID_RESP2 0x04EFF701


struct msg1 {
	uint16_t error;
	uint8_t status;
	uint32_t current;
	uint8_t soc;
} __attribute__ (( __packed__ ));


struct msg2 {
	uint16_t cycle_count;
	uint16_t temperature;
	uint32_t voltage;
} __attribute__ (( __packed__ ));


static struct api_can_addr can_filter[] =
{
	{ PHILIPPI_ID_QUERY, 0xffffffff },
};




static rv fn_recv(const struct module *mod, enum can_address_mode_t fmt, uint32_t can_id, void *data, size_t len)
{
	if(can_id == PHILIPPI_ID_QUERY) {

		struct accum_batt_data ac;
		bci_get_accum_data(&ac);

		struct msg1 m1;
		m1.error = ac.error_flags;
		m1.status = ac.status_flags;
		m1.current = ac.I;
		m1.soc = ac.soc;

		struct msg2 m2;
		m2.cycle_count = 0;
		m2.temperature = ac.T_batt;
		m2.voltage = ac.U;

		can_tx(&can0, CAN_ADDRESS_MODE_EXT, PHILIPPI_ID_RESP1, &m1, sizeof m1);
		can_tx(&can0, CAN_ADDRESS_MODE_EXT, PHILIPPI_ID_RESP2, &m2, sizeof m2);
	}

	return RV_OK;
}



static rv fn_init(const struct module *mod)
{
	module_printd(mod, "Philipi init\n");
	can_set_speed(&can0, CAN_SPEED_250_KB);
	return RV_OK;
}


MOD_REGISTER(
	.id = 2,
	.name = "philippi",
	.description = "Philippi RBUS support at 250 KB/sec",
	.revision = 100,
	.options = MOD_OP_USERSELECT,
	.can_filter = can_filter,
	.fn_recv = fn_recv,
	.fn_init = fn_init,
)

/*
 * End
 */

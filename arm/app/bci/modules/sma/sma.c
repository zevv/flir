
/*
 * Interfacing to SMA Sunny Island
 */

#include <limits.h>
#include <stdint.h>
#include <string.h>

#include "bios/bios.h"
#include "bios/printd.h"
#include "bios/sysinfo.h"

#include "app/bci/bci-api.h"
#include "app/bci/modules.h"


struct sma_351 {
	uint16_t battery_charge_voltage;          /* 0.1 V,(41..63) */
	int16_t  dc_charge_current_limitation;    /* 0.1 A (0..1200) */
	int16_t  dc_discharge_current_limitation; /* 0.1 A (0..1200) */
	uint16_t discharge_voltage_limit;         /* 0.1 V (41..48) */
};

struct sma_355 {
	uint16_t soc_value;                       /* % (0..100) */
	uint16_t soh_value;                       /* % (0..100) */
	uint16_t hires_soc;                       /* % (0..100) */
};

struct sma_356 {
	int16_t battery_voltage;                  /* 0.01 V */
	int16_t battery_current;                  /* 0.1 A */
	int16_t battery_temperature;              /* 0.1 °C */
};

struct sma_35a {
	uint32_t alarms;
	uint32_t warnings;
};

struct sma_35e {
	char manufacturer_name[8];
};

struct sma_35f {
	uint16_t bat_type;
	uint16_t bms_version;
	uint16_t bat_capacity;
	uint16_t reserved_manufacturer_id;
};

static int float_mode = 0;


#define ALARM_RAISE 0x01
#define ALARM_CLEAR 0x02
#define ALARM_MASK 0x03

enum alarm_code {
	ALARM_GENERAL,
	ALARM_BATTERY_HIGH_VOLTAGE,
	ALARM_BATTERY_LOW_VOLTAGE,
	ALARM_BATTERY_HIGH_TEMP,
	ALARM_BATTERY_LOW_TEMP,
	ALARM_BATTERY_HIGH_CHARGE_TEMP,
	ALARM_BATTERY_LOW_CHARGE_TEMP,
	ALARM_BATTERY_HIGH_CURRENT,
	ALARM_BATTERY_HIGH_CURRENT_CHARGE,
	ALARM_CONTACTOR,
	ALARM_SHORT_CIRCUIT,
	ALARM_BMS_INTERNAL,
	ALARM_CELL_IMBALANCE,
	ALARM_RESERVED_1,
	ALARM_RESERVED_2,
	ALARM_GENERATOR,
};



static rv fn_init(const struct module *mod)
{
	printd("SMA init\n");
	can_set_speed(&can0, CAN_SPEED_250_KB);
	//api_switch_error_inhibit(mod, 30);
	return RV_OK;
}


static void send_data(const struct module *mod)
{
	struct accum_batt_data ac;
	struct layout_info layout;

	bci_get_accum_data(&ac);
	bci_get_layout(&layout);

	int I_req = ac.I_request * 10 / 16;

	/* check if we want to go to float mode */

	if(I_req == 0 && ac.U < 13.8 * layout.n_series ) { 
		if(float_mode == 0) printd("float mode enable\n");
		float_mode = 1;
	} else if(I_req > 0) {
		if(float_mode == 1) printd("float mode disable\n");
		float_mode = 0;
	}

	int U_req;

	if(float_mode) {
		U_req = 138 * layout.n_series;
		I_req = ac.Q_design * 0.01;
	} else {
		U_req = 144 * layout.n_series;
		I_req = ac.I_request / 16;
	}

	{
		struct sma_351 msg;
		msg.battery_charge_voltage = U_req;
		msg.dc_charge_current_limitation = I_req;
		msg.dc_discharge_current_limitation = ac.Q_design * 0.01 * 3;
		msg.discharge_voltage_limit = (layout.n_series * 110);
		can_tx(&can0, CAN_ADDRESS_MODE_STD, 0x351, &msg, sizeof(msg));
	}

	{
		struct sma_355 msg;
		msg.soc_value = ac.soc;
		msg.soh_value = 100;
		msg.hires_soc = 0;
		can_tx(&can0, CAN_ADDRESS_MODE_STD, 0x355, &msg, sizeof(msg));
	}

	{
		struct sma_356 msg;
		msg.battery_voltage = 100 * ac.U / 1024;
		msg.battery_current = (int32_t)ac.I / 100;
		msg.battery_temperature = 10 * (int32_t)ac.T_batt / 8;
		can_tx(&can0, CAN_ADDRESS_MODE_STD, 0x356, &msg, sizeof(msg));
	}

	{
		struct sma_35a msg;
		msg.warnings = 0;
		msg.alarms = 0;
		uint16_t errors = ac.error_flags;

		/* Map the status as an additional error bit */

		//uint8_t switch_is_online;
		//api_switch_main_status(mod, &switch_is_online);
		//if(!switch_is_online) errors |= (1<<6);

		/* Map super-b battery error flags to SMA alarms */

		static int error_map[] = {
			[0] = ALARM_BATTERY_HIGH_VOLTAGE,
			[1] = ALARM_BATTERY_LOW_VOLTAGE,
			[2] = ALARM_BATTERY_HIGH_CURRENT_CHARGE,
			[3] = ALARM_BATTERY_HIGH_CURRENT,
			[4] = ALARM_BATTERY_HIGH_TEMP,
			[5] = ALARM_BMS_INTERNAL, /* Fuse, not used */
			[6] = ALARM_CONTACTOR, 
		};

		for(int i=0; i<=6; i++) {
			int alarm = error_map[i];
			int shift = (alarm << 1);
			if(errors & (1<<i)) {
				msg.alarms |= (ALARM_RAISE << shift);
			} else {
				msg.alarms |= (ALARM_CLEAR << shift);
			}
		}

		//printd("SMA errors %04x: 0x%08x online %d\n", errors, msg.alarms, switch_is_online);
		can_tx(&can0, CAN_ADDRESS_MODE_STD, 0x35a, &msg, sizeof(msg));
	}

	{
		struct sma_35e msg;
		memcpy(msg.manufacturer_name, "Super B ", 8);
		can_tx(&can0, CAN_ADDRESS_MODE_STD, 0x35e, &msg, sizeof(msg));
	}

	{
		struct sysinfo_version ver;
		sysinfo_get_version(&ver);

		struct sma_35f msg;
		msg.bat_type = 0;
		msg.bms_version = ver.maj * 10000 + ver.min * 100;
		msg.bat_capacity = ac.Q_design / 3600.0;
		msg.reserved_manufacturer_id = 0;
		can_tx(&can0, CAN_ADDRESS_MODE_STD, 0x35f, &msg, sizeof(msg));
	}

}


static rv fn_timer(const struct module *mod)
{
	static uint8_t ticks = 0;
	ticks++;
	if(ticks >= 20) {
		send_data(mod);
		ticks = 0;
	}
	return RV_OK;
}


MOD_REGISTER(
	.id = 3,
	.name = "sma",
	.description = "SMA Solar Technology",
	.revision = 100,
	.options = MOD_OP_USERSELECT,
	.fn_init = fn_init,
	.fn_timer = fn_timer,
)



#ifndef lib_battdb_battdb_h
#define lib_battdb_battdb_h

enum cell_id {
	CELL_ID_ANR26650,
	CELL_ID_LFP26650,
	CELL_ID_TRACTIE50,
	CELL_ID_TRACTIE100,
	CELL_ID_TRACTIE160,
};

enum batt_id {
	BATT_ID_SB12V2600P_AC,
	BATT_ID_SB12V5200P_BC,
	BATT_ID_SB12V7800P_CC,
	BATT_ID_SB12V10P_DC,
	BATT_ID_SB12V15P_EC,
	BATT_ID_SB12V20P_FC,
	BATT_ID_SB12V15P_SC,
	BATT_ID_SB12V20P_SC,
	BATT_ID_SB12V25P_SC,
	BATT_ID_RX7_12L,
	BATT_ID_SB12V3200E_AC,
	BATT_ID_SB12V6400E_BC,
	BATT_ID_SB12V10E_CC,
	BATT_ID_SB12V13E_DC,
	BATT_ID_SB12V19E_EC,
	BATT_ID_SB12V26E_FC,
	BATT_ID_SB12V32E_GC,
	BATT_ID_SB6V20E_CC,
	BATT_ID_SB6V5200P_AC,
	BATT_ID_SB12V32E_SC,
	BATT_ID_SB12V19E_SC,
	BATT_ID_SB12V26E_SC,
	BATT_ID_SB12V50E_XC,
	BATT_ID_SB12V100_ZC,
	BATT_ID_SB12V160E_ZC,
	BATT_ID_SB12V50E_MC,
	BATT_ID_SB12V100E_NC,
	BATT_ID_SB12V100E_ZC_AD,
	BATT_ID_SB12V160E_OC,
};

struct cell_param {
	const char *name;
	voltage U_open;
	voltage U_charge;
	voltage U_empty;
	charge Q;
	current I_max_charge;
	current I_max_discharge;
	resistance R;
};

struct batt_param {
	const char *ean;
	const char *id;
	enum cell_id cell;
	uint8_t series;
	uint8_t parallel;
};


#endif

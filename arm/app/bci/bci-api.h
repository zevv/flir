#ifndef bci_api_h
#define bci_api_h

#define BATT_ID_MIN 1u
#define BATT_ID_MAX 127u

enum bci_state {
	BCI_STATE_INIT,
	BCI_STATE_RUNNING,
	BCI_STATE_SCANNING
};

enum batt_state {
	BATT_STATE_UNKNOWN,
	BATT_STATE_SCANNING,
	BATT_STATE_TESTING,
	BATT_STATE_AVAILABLE,
	BATT_STATE_UNAVAILABLE,
	BATT_STATE_MISSING
};


struct layout_info {
	uint8_t n_series;
	uint8_t n_parallel;
};


/*
 * Single battery data
 */

struct batt_data {
	uint8_t status_flags;
	uint32_t identity;
	uint16_t error;

	voltage U;

	current I;
	current I_request;
	current I_charge_max;

	temperature T_batt;

	charge Q_design;
	uint8_t soc;
	uint16_t cycle_count;

	struct {
		voltage U;
	} cell[4];
};

/*
 * Accumulated data for all batteries connected to the BCI
 */

struct accum_batt_data {
	uint8_t status_flags;
	uint16_t error_flags;
	uint16_t last_error_flags;

	voltage U;

	current I;
	current I_charge_max;
	current I_request;

	temperature T_batt;

	charge Q_design;

	uint8_t soc;
	uint16_t cycle_count;
};


rv bci_get_state(enum bci_state *state);
rv bci_full_scan(uint8_t node_id_first, uint8_t node_id_last);
rv bci_get_batt_state(enum batt_state *state_list, uint8_t count);
rv bci_get_batt_data(uint8_t batt_id, struct batt_data *data);
rv bci_get_accum_data(struct accum_batt_data *data);
rv bci_get_layout(struct layout_info *li);
rv bci_set_layout(struct layout_info *li);


#endif

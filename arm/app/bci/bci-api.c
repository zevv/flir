
#include <string.h>

#include "bios/bios.h"
#include "lib/config.h"
#include "lib/canopen/inc/canopen.h"
#include "app/bci/bci-api.h"
#include "app/bci/can-scanner.h"

rv bci_get_state(enum bci_state *state)
{
	enum can_scanner_state scanner_state;
	uint8_t perc_done;
	rv r = can_scanner_get_state(&scanner_state, &perc_done);

	if(r == RV_OK) {
		if(scanner_state == CAN_SCANNER_STATE_SCAN) {
			*state = BCI_STATE_SCANNING;
		} else {
			*state = BCI_STATE_RUNNING;
		}
	}
	return r;
}


rv bci_full_scan(uint8_t node_id_first, uint8_t node_id_last)
{
	return can_scanner_full_scan(node_id_first, node_id_last);
}


rv bci_get_batt_state(enum batt_state *state_list, uint8_t count)
{
	rv r = RV_OK;

	if(count <= BATT_ID_MAX) {
		uint8_t i;
		for(i=0; i<count; i++) {
			r = can_scanner_get_batt_state(i, &state_list[i]);
		}
	} else {
		r = RV_EINVAL;
	}

	return r;
}


rv bci_get_layout(struct layout_info *li)
{
	return config_read("layout", li, sizeof(*li));
}


rv bci_set_layout(struct layout_info *li)
{
	return config_write("layout", li, sizeof(*li));
}


rv bci_get_batt_data(uint8_t batt_id, struct batt_data *data)
{
	return can_scanner_get_batt_data(batt_id, data);
}


rv bci_get_accum_data(struct accum_batt_data *data)
{
	rv r;

	memset(data, 0, sizeof(*data));

	/* Todo: for now we take the average for all values, this will need to
	 * be more intelligent: some are avg, some are min and some are max */

	uint8_t count = 0;
	voltage U_total = 0.0f;
	current I_total = 0.0f;
	current I_request_total = 0.0;
	current I_charge_max_total = 0.0;
	charge Q_design_total = 0.0;
	temperature T_total = 0.0f;
	uint32_t soc_total = 0;
	uint32_t cycle_count_total = 0;

	enum batt_state state_list[BATT_ID_MAX+1u];
	r = bci_get_batt_state(state_list, BATT_ID_MAX);

	if(r == RV_OK) {
	
		uint8_t i;
		for(i=0; i<=BATT_ID_MAX; i++) {

			if(state_list[i] == BATT_STATE_AVAILABLE) {

				struct batt_data bd;
				r = bci_get_batt_data(i, &bd);

				if(r == RV_OK) {
					U_total += bd.U;
					I_total += bd.I;
					I_request_total += bd.I_request;
					I_charge_max_total += bd.I_charge_max;
					T_total += bd.T_batt;
					Q_design_total += bd.Q_design;
					soc_total += bd.soc;
					cycle_count_total += bd.cycle_count;

					count ++;
				} else {
					break;
				}
			}
		}

		if((r == RV_OK) && (count > 0u)) {

			data->U = U_total / (voltage)count;
			data->I = I_total / (current)count;
			data->I_request = I_request_total / (current)count;
			data->I_charge_max = I_charge_max_total / (current)count;
			data->T_batt = T_total / (temperature)count;
			data->soc = (uint8_t)(soc_total / (uint32_t)count);
			data->cycle_count = (uint16_t)(cycle_count_total / (uint32_t)count);
			data->Q_design = Q_design_total / (charge)count;

		} else {
			r = RV_ENODEV;
		}
	}

	return r;
}


/*
 * End
 */

#ifndef can_scanner_h
#define can_scanner_h

enum can_scanner_state {
	CAN_SCANNER_STATE_SCAN,
	CAN_SCANNER_STATE_POLL,
};

rv can_scanner_init(CoHandle ch);
rv can_scanner_pause(void);
rv can_scanner_resume(void);
rv can_scanner_full_scan(uint8_t node_id_first, uint8_t node_id_last);
rv can_scanner_get_state(enum can_scanner_state *state, uint8_t *perc_done);

rv can_scanner_get_batt_state(uint8_t node_id, enum batt_state *state);
rv can_scanner_get_batt_data(uint8_t node_id, struct batt_data *data);

#endif

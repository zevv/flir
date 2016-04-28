#ifndef lepton_h
#define lepton_h

enum lepton_mod {
	LEPTON_MOD_AGC = 0x01,
	LEPTON_MOD_SYS = 0x02,
	LEPTON_MOD_VID = 0x03,
};

enum lepton_cmd_type {
	LEPTON_CMD_TYPE_GET = 0x00,
	LEPTON_CMD_TYPE_SET = 0x01,
	LEPTON_CMD_TYPE_RUN = 0x02,
};

enum lepton_cmd_id {
	LEPTON_CMD_ID_AGC = 0x00,
	LEPTON_CMD_ID_UPTIME = 0x0c,
	LEPTON_CMD_ID_FPA_TEMPERATURE = 0x14,
	LEPTON_CMD_ID_TELEMETRY = 0x18,
	LEPTON_CMD_ID_RUN_FCC = 0x40,
};

void lepton_run(enum lepton_mod mod, uint8_t cmd_id);
void lepton_get(enum lepton_mod mod, uint8_t cmd_id, void *data, size_t len);
void lepton_set_32(enum lepton_mod mod, uint8_t cmd_id, uint32_t val);
void lepton_set_16(enum lepton_mod mod, uint8_t cmd_id, uint16_t val);

extern uint8_t img[60][64];

#endif

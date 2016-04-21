#ifndef app_device_state_h
#define app_device_state_h

enum device_state {
	DEVICE_STATE_INIT,
	DEVICE_STATE_OFF,
	DEVICE_STATE_ON1,
	DEVICE_STATE_ON2,
	DEVICE_STATE_ON,
	DEVICE_STATE_PRE1,
	DEVICE_STATE_PRE
};

enum device_cmd {
	DEVICE_CMD_OFF,
	DEVICE_CMD_ON,
	DEVICE_CMD_PRECHARGE,
	DEVICE_CMD_RESET,
	DEVICE_CMD_TICK_10HZ,
};


rv device_state_get(enum device_state *state_out);
rv device_state_cmd(enum device_cmd cmd);



#endif



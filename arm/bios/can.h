
#ifndef bios_can_h
#define bios_can_h

enum can_address_mode_t {
	CAN_ADDRESS_MODE_STD,
	CAN_ADDRESS_MODE_EXT,
};

enum can_flags {
	CAN_STATUS_BUS_ON,
	CAN_STATUS_BUS_OFF,
};

enum can_speed {
	CAN_SPEED_10_KB,
	CAN_SPEED_20_KB,
	CAN_SPEED_50_KB,
	CAN_SPEED_100_KB,
	CAN_SPEED_125_KB,
	CAN_SPEED_250_KB,
	CAN_SPEED_500_KB,
	CAN_SPEED_800_KB,
	CAN_SPEED_1000_KB,
};

struct can_stats {
	enum can_flags flags;

	uint32_t rx_total;
	uint32_t rx_err;
	uint32_t rx_xrun;

	uint32_t tx_total;
	uint32_t tx_err;
	uint32_t tx_xrun;
};

typedef uint32_t can_addr_t;

struct dev_can;

rv can_init(struct dev_can *dev);
rv can_set_speed(struct dev_can *dev, enum can_speed speed);
rv can_tx(struct dev_can *dev, enum can_address_mode_t fmt, can_addr_t addr, const void *data, size_t len);
rv can_get_stats(struct dev_can *dev, struct can_stats *stats);

#endif

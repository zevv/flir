#ifndef drv_stm32_can_h
#define drv_stm32_can_h

#include "drv/can/can.h"
#include "stm32f1xx_hal.h"

struct drv_can_stm32_data {
	CAN_TypeDef *bus;

	CAN_HandleTypeDef _hcan;
	CanTxMsgTypeDef _tx_msg;
	CanRxMsgTypeDef _rx_msg;
	uint16_t _tx_total;
	uint16_t _tx_err;
	uint16_t _tx_xrun;
	uint16_t _rx_total;
	uint16_t _rx_err;
	uint16_t _rx_xrun;
};

extern const struct drv_can drv_can_stm32;

#endif


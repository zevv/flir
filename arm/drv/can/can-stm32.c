
#include <stdint.h>
#include <string.h>

#include "bios/bios.h"
#include "bios/can.h"
#include "bios/gpio.h"
#include "bios/evq.h"
#include "bios/printd.h"
#include "arch/stm32/arch.h"
#include "drv/can/can.h"
#include "drv/can/can-stm32.h"

/*
 * The STM32 HAL * layer does not allow us to store an opaque pointer with the
 * CAN callbacks, so we need to store references to the CAN device objects in a
 * global table to be able to find a reference to the proper struct dev_can
 * from interrupt context.
 */

static struct dev_can *dev_list[2];

static rv init(struct dev_can *dev)
{
	struct drv_can_stm32_data *dd = dev->drv_data;

	/* Configure GPIO for can operation */

	if(dd->bus == CAN1) {
		__HAL_RCC_CAN1_CLK_ENABLE();
		__HAL_RCC_GPIOA_CLK_ENABLE();

		/* Configure CAN GPIO pins */
		
		GPIO_InitTypeDef gpio;

		gpio.Pin = GPIO_PIN_11;
		gpio.Mode = GPIO_MODE_AF_INPUT;
		gpio.Pull = GPIO_PULLUP;
		gpio.Speed = GPIO_SPEED_HIGH;
		HAL_GPIO_Init(GPIOA, &gpio);

		gpio.Pin = GPIO_PIN_12;
		gpio.Mode = GPIO_MODE_AF_PP;
		gpio.Pull = GPIO_PULLUP;
		gpio.Speed = GPIO_SPEED_HIGH;
		HAL_GPIO_Init(GPIOA, &gpio);
		
		HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
		HAL_NVIC_EnableIRQ(CAN1_RX1_IRQn);
		HAL_NVIC_EnableIRQ(CAN1_TX_IRQn);

		dev_list[0] = dev;
	}

	if(dd->bus == CAN2) {
		__HAL_RCC_CAN1_CLK_ENABLE();
		__HAL_RCC_CAN2_CLK_ENABLE();
		__HAL_RCC_GPIOB_CLK_ENABLE();

		/* Configure CAN GPIO pins */
		
		GPIO_InitTypeDef gpio;

		gpio.Pin = GPIO_PIN_12;
		gpio.Mode = GPIO_MODE_AF_INPUT;
		gpio.Pull = GPIO_PULLUP;
		gpio.Speed = GPIO_SPEED_HIGH;
		HAL_GPIO_Init(GPIOB, &gpio);

		gpio.Pin = GPIO_PIN_13;
		gpio.Mode = GPIO_MODE_AF_PP;
		gpio.Pull = GPIO_PULLUP;
		gpio.Speed = GPIO_SPEED_HIGH;
		HAL_GPIO_Init(GPIOB, &gpio);
	
		HAL_NVIC_EnableIRQ(CAN2_RX0_IRQn);
		HAL_NVIC_EnableIRQ(CAN2_RX1_IRQn);
		HAL_NVIC_EnableIRQ(CAN2_TX_IRQn);

		dev_list[1] = dev;
	}

	/* Configure CAN interface */
	
	CAN_HandleTypeDef *hcan = &dd->_hcan;

	hcan->pTxMsg = &dd->_tx_msg;
	hcan->pRxMsg = &dd->_rx_msg;

	hcan->Instance = dd->bus;
	hcan->Init.TTCM = DISABLE;
	hcan->Init.ABOM = DISABLE;
	hcan->Init.AWUM = DISABLE;
	hcan->Init.NART = DISABLE;
	hcan->Init.RFLM = DISABLE;
	hcan->Init.TXFP = DISABLE;
	hcan->Init.Mode = CAN_MODE_NORMAL;
	hcan->Init.SJW = CAN_SJW_1TQ;
	hcan->Init.BS1 = CAN_BS1_6TQ;
	hcan->Init.BS2 = CAN_BS2_1TQ;
	hcan->Init.Prescaler = 18;
	HAL_CAN_Init(hcan);

	CAN_FilterConfTypeDef sFilterConfig;
	sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
	sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
	sFilterConfig.FilterIdHigh = 0x0000;
	sFilterConfig.FilterIdLow = 0x0000;
	sFilterConfig.FilterMaskIdHigh = 0x0000;
	sFilterConfig.FilterMaskIdLow = 0x0000;
	sFilterConfig.FilterActivation = ENABLE;

	if(dd->bus == CAN1) {
		sFilterConfig.BankNumber = 0;
		sFilterConfig.FilterNumber = 0;
		sFilterConfig.FilterFIFOAssignment = 0;
	}
	
	if(dd->bus == CAN2) {
		sFilterConfig.BankNumber = 0;
		sFilterConfig.FilterNumber = 14;
		sFilterConfig.FilterFIFOAssignment = 1;
	}

	HAL_CAN_ConfigFilter(hcan, &sFilterConfig);
	
	if(dd->bus == CAN1) {
		HAL_CAN_Receive_IT(hcan, CAN_FIFO0);
	}
		
	if(dd->bus == CAN2) {
		HAL_CAN_Receive_IT(hcan, CAN_FIFO0);
	}

	return RV_OK;
}


static rv set_speed(struct dev_can *dev, enum can_speed speed)
{
	struct drv_can_stm32_data *dd = dev->drv_data;
	CAN_HandleTypeDef *hcan = &dd->_hcan;
	rv r;

	hcan->Init.SJW = CAN_SJW_1TQ;
	hcan->Init.BS1 = CAN_BS1_6TQ;
	hcan->Init.BS2 = CAN_BS2_1TQ;
	hcan->Init.Prescaler = 4;

	HAL_StatusTypeDef rc = HAL_CAN_Init(hcan);

	switch(rc) {
		case HAL_ERROR: r = RV_EIO; break;
		case HAL_BUSY: r = RV_EBUSY; break;
		case HAL_TIMEOUT: r = RV_ETIMEOUT; break;
		default: r = RV_OK; break;
	}

	return r;
}


static rv tx(struct dev_can *can, enum can_address_mode_t fmt, can_addr_t addr, const void *data, size_t len)
{
	struct drv_can_stm32_data *dd = can->drv_data;
	CAN_HandleTypeDef *hcan = &dd->_hcan;
	CanTxMsgTypeDef *tx_msg = &dd->_tx_msg;

	if(fmt == CAN_ADDRESS_MODE_EXT) {
		tx_msg->ExtId = addr;
		tx_msg->IDE = CAN_ID_EXT;
	} else {
		tx_msg->StdId = addr;
		tx_msg->IDE = CAN_ID_STD;
	}

	tx_msg->RTR = CAN_RTR_DATA;
	tx_msg->DLC = len;
	memcpy(tx_msg->Data, data, len);

	HAL_StatusTypeDef r = HAL_CAN_Transmit_IT(hcan);
	return hal_status_to_rv(r);
}


static rv get_stats(struct dev_can *dev, struct can_stats *stats)
{
	struct drv_can_stm32_data *dd = dev->drv_data;
	CAN_HandleTypeDef *hcan = &dd->_hcan;
	
	uint32_t code = HAL_CAN_GetError(hcan);

	stats->tx_total = dd->_tx_total;
	stats->tx_err = dd->_tx_err;
	stats->tx_xrun = dd->_tx_xrun;
	
	stats->rx_total = dd->_rx_total;
	stats->rx_err = dd->_rx_err;
	stats->rx_xrun = dd->_rx_xrun;

	stats->flags = code;

	return RV_OK;
}


void HAL_CAN_RxCpltCallback(CAN_HandleTypeDef *hcan)
{
	struct dev_can *dev = (hcan->Instance == CAN1) ? dev_list[0] : dev_list[1];
	CanRxMsgTypeDef *rx_msg = hcan->pRxMsg;
	struct drv_can_stm32_data *dd = dev->drv_data;

	event_t e;
	e.type = EV_CAN;
	e.can.dev = dev;
	e.can.extended = (rx_msg->IDE == CAN_ID_EXT);
	e.can.id = rx_msg->StdId;
	e.can.len = rx_msg->DLC;
	memcpy(e.can.data, rx_msg->Data, rx_msg->DLC);
	evq_push(&e);
	
	dd->_rx_total ++;

	HAL_CAN_Receive_IT(hcan, CAN_FIFO0);
}


void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
	uint32_t code = HAL_CAN_GetError(hcan);
	printd("CAN error");
	if(code & HAL_CAN_ERROR_EWG ) printd(" EWG");
	if(code & HAL_CAN_ERROR_EPV ) printd(" EPV");
	if(code & HAL_CAN_ERROR_BOF ) printd(" BOF");
	if(code & HAL_CAN_ERROR_STF ) printd(" Stuff");
	if(code & HAL_CAN_ERROR_FOR ) printd(" Form");
	if(code & HAL_CAN_ERROR_ACK ) printd(" Acknowledgmen");
	if(code & HAL_CAN_ERROR_BR  ) printd(" Bit recessive");
	if(code & HAL_CAN_ERROR_BD  ) printd(" LEC dominant");
	if(code & HAL_CAN_ERROR_CRC ) printd(" LEC transfer");
	printd("\n");
}


void HAL_CAN_TxCpltCallback(CAN_HandleTypeDef *hcan)
{
	struct dev_can *dev = (hcan->Instance == CAN1) ? dev_list[0] : dev_list[1];
	if(dev) {
		struct drv_can_stm32_data *dd = dev->drv_data;
		dd->_tx_total ++;
	}
}


void can1_irq(void)
{
	struct drv_can_stm32_data *dd = dev_list[0]->drv_data;
	HAL_CAN_IRQHandler(&dd->_hcan);
}

void can2_irq(void)
{
	struct drv_can_stm32_data *dd = dev_list[1]->drv_data;
	HAL_CAN_IRQHandler(&dd->_hcan);
}


const struct drv_can drv_can_stm32 = {
	.init = init,
	.set_speed = set_speed,
	.tx = tx,
	.get_stats = get_stats,
};

/*
 * End
 */

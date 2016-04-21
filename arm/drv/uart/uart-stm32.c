
#include <unistd.h>
#include <stdint.h>

#include "bios/bios.h"
#include "bios/uart.h"
#include "drv/uart/uart-stm32.h"
#include "bios/evq.h"
#include "bios/printd.h"

#include "stm32f1xx_hal.h"

#define RB_SIZE 512

struct ringbuffer_t {
	volatile uint16_t head;
	volatile uint16_t tail;
	volatile uint8_t buf[RB_SIZE];
};

static struct ringbuffer_t rb = { 0, 0 };

static rv init(struct dev_uart *dev)
{
	struct drv_uart_stm32_data *dd = dev->drv_data;

	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_USART1_CLK_ENABLE();

	GPIO_InitTypeDef GPIO_InitStruct;

	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
	GPIO_InitStruct.Pin = GPIO_PIN_9;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	UART_HandleTypeDef *UartHandle = &dd->_huart;
	UartHandle->Instance        = USART1;

	UartHandle->Init.BaudRate   = 57600;
	UartHandle->Init.WordLength = UART_WORDLENGTH_8B;
	UartHandle->Init.StopBits   = UART_STOPBITS_1;
	UartHandle->Init.Parity     = UART_PARITY_NONE;
	UartHandle->Init.HwFlowCtl  = UART_HWCONTROL_NONE;
	UartHandle->Init.Mode       = UART_MODE_TX_RX;

	HAL_UART_Init(UartHandle);

	/* Enable Rx interrupt */

	USART1->CR1 |= USART_CR1_RXNEIE;
	NVIC_EnableIRQ(USART1_IRQn);

	dd->_ready = 1;

	return RV_OK;
}


static rv tx(struct dev_uart *dev, uint8_t c)
{
	struct drv_uart_stm32_data *dd = dev->drv_data;

	uint16_t head = (rb.head + 1) % RB_SIZE;
	while(dd->_ready && (rb.tail == head));
	rb.buf[rb.head] = c;
	rb.head = head;

	USART1->CR1 |= USART_CR1_TXEIE;

	return RV_OK;
}


void usart1_irq(void)
{
	if(USART1->SR & USART_SR_RXNE) {
		char c = USART1->DR;
		event_t ev;
		ev.type = EV_UART;
		ev.uart.data = c;
		evq_push(&ev);
	}

	if(USART1->SR & USART_SR_TXE) {
		if(rb.tail != rb.head) {
			USART1->DR = rb.buf[rb.tail];
			rb.tail = (rb.tail + 1) % RB_SIZE;
		}
		if(rb.tail == rb.head) {
			USART1->CR1 &= ~USART_CR1_TXEIE;
		}
	}
}

struct drv_uart drv_uart_stm32 = {
	.init = init,
	.tx = tx,
};

/*
 * End
 */

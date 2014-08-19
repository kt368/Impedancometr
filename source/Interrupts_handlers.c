#include "Interrupt_handlers.h"

uint32_t i2s_fifo[8];

volatile uint32_t systick_ten_millis=0;

void SysTick_Handler(void)
{
	systick_ten_millis++;
}

void I2S_IRQHandler(void)
{
	extern uint8_t I2S_complete;
	NVIC_DisableIRQ(I2S_IRQn);
	NVIC_ClearPendingIRQ(I2S_IRQn);
	i2s_fifo[0] = LPC_I2S ->I2SRXFIFO;
	i2s_fifo[1] = LPC_I2S ->I2SRXFIFO;
	i2s_fifo[2] = LPC_I2S ->I2SRXFIFO;
	i2s_fifo[3] = LPC_I2S ->I2SRXFIFO;
	i2s_fifo[4] = LPC_I2S ->I2SRXFIFO;
	i2s_fifo[5] = LPC_I2S ->I2SRXFIFO;
	i2s_fifo[6] = LPC_I2S ->I2SRXFIFO;
	i2s_fifo[7] = LPC_I2S ->I2SRXFIFO;
	I2S_complete = 1;
	NVIC_EnableIRQ(I2S_IRQn);
}
	

#include "Interrupt_handlers.h"

uint32_t i2s_fifo[8];

volatile uint32_t systick_ten_millis=0;

void SysTick_Handler(void)
{
	systick_ten_millis++;
}



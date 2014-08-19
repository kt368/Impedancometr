/*----------------------------------------------------------------------------
 * Name:    Retarget.c
 * Purpose: 'Retarget' layer for target-dependent low level functions
 * Note(s):
 *----------------------------------------------------------------------------*/

#include <stdio.h>
#include "Retarget.h"

#pragma import(__use_no_semihosting_swi)

struct __FILE { int handle; /* Add whatever you need here */ };
FILE __stdout;
FILE __stdin;
FILE __stderr;

extern void SW_UART_push(uint8_t item);
extern uint8_t SW_UART_pop(void);
uint_fast8_t UartWithFifo = 0;

void UART_SendByte_with_FIFO(LPC_UART_TypeDef* UARTx, uint8_t data)
{
	SW_UART_push(data);
  UART_IntConfig(LPC_UART0,UART_INTCFG_THRE,ENABLE);			//Enable UART0 THRE Interrupt
	//NVIC_EnableIRQ(UART0_IRQn);															//Enable UART0 Interrupts
	if (UART_CheckBusy(LPC_UART0) == RESET)
	{
		LPC_UART0->THR = SW_UART_pop();
	}
}

int fputc(int c, FILE *f) {
	if (UartWithFifo == 1)
	{
		if (c == '\n')  {
			UART_SendByte_with_FIFO(LPC_UART0, '\r');
		}
		UART_SendByte_with_FIFO(LPC_UART0, c);
	}
	else
	{
		while(!(LPC_UART0->LSR & UART_LSR_THRE)){};
		if (c == '\n')  {
			UART_SendByte(LPC_UART0, '\r');
		}
		UART_SendByte(LPC_UART0, c);
	}
  return (c);
}


int fgetc(FILE *f) {
  return (UART_ReceiveByte(LPC_UART0));
}


int ferror(FILE *f) {
  /* Your implementation of ferror */
  return EOF;
}

void _ttywrch(int c) {
	if (UartWithFifo == 1)
		UART_SendByte_with_FIFO(LPC_UART0, c);
	else
		while(!(LPC_UART0->LSR & UART_LSR_THRE)){};
		UART_SendByte(LPC_UART0, c);
}


void _sys_exit(int return_code) {
label:  goto label;  /* endless loop */
}

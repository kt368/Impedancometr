#include <LPC17XX.h>                       /* LPC17XX definitions         */
#include "lpc17xx_uart.h"
#define SW_UART_FIFO_SIZE 128

struct SW_UART_FIFO_STRUCT_TYPE{
  uint16_t		size;
	uint16_t		count;
	uint16_t		last;
	uint16_t		first;
};



uint8_t SW_UART_FIFO_BUF[SW_UART_FIFO_SIZE];

void SW_UART_push(uint8_t item);
uint8_t SW_UART_pop(void);

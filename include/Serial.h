#include "lpc17xx_uart.h"
#include <string.h>
#include "System.h"

UART_CFG_Type UART_ConfigStruct;

void SER_Init(uint32_t baudrate);

extern void wait(uint32_t);

struct SW_UART_FIFO_STRUCT_TYPE{
  uint16_t		size;
	uint16_t		count;
	uint16_t		last;
	uint16_t		first;
};

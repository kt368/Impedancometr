#include "SW_FIFO.h"

struct SW_UART_FIFO_STRUCT_TYPE SW_UART_FIFO_STRUCT = {SW_UART_FIFO_SIZE,0,0,0};

void SW_UART_push(uint8_t item)
{

if(SW_UART_FIFO_STRUCT.count == SW_UART_FIFO_STRUCT.size) return;
	UART_IntConfig(LPC_UART0,UART_INTCFG_THRE,DISABLE);
	SW_UART_FIFO_STRUCT.count++;
	SW_UART_FIFO_BUF[SW_UART_FIFO_STRUCT.last++] = item;
	if(SW_UART_FIFO_STRUCT.last >= SW_UART_FIFO_STRUCT.size) SW_UART_FIFO_STRUCT.last = 0;
	UART_IntConfig(LPC_UART0,UART_INTCFG_THRE,ENABLE);
}
uint8_t SW_UART_pop()
{
	uint8_t item;
  if(SW_UART_FIFO_STRUCT.count == 0) return 0;
  SW_UART_FIFO_STRUCT.count--;
  item = SW_UART_FIFO_BUF[SW_UART_FIFO_STRUCT.first++];
  if(SW_UART_FIFO_STRUCT.first == SW_UART_FIFO_STRUCT.size) SW_UART_FIFO_STRUCT.first = 0;
  return item;
}

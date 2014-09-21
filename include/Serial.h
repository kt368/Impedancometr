#include <LPC17XX.h>
#include <lpc_types.h>
#include <stdio.h>
#include "lpc17xx_uart.h"
#include "GPIO_signals.h"
#include <string.h>
#include "Global_header.h"
#include "System.h"
#include "AD9833.h"
#include "lpc17xx_i2s.h"

UART_CFG_Type UART_ConfigStruct;

void SER_Init(uint32_t baudrate);
extern void test(void);
extern void CalibrateMagPhaseCalcTheoretic(mag_ph_calc_calibr_struct_t *);
extern void wait(uint32_t);

struct SW_UART_FIFO_STRUCT_TYPE{
  uint16_t		size;
	uint16_t		count;
	uint16_t		last;
	uint16_t		first;
};

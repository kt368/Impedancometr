#include "Global_header.h"
#include <stdio.h>
#include "lpc17xx_gpio.h"

#include "lpc17xx_uart.h"
#include <LPC17XX.h>                       /* LPC17XX definitions         */
#include <system_LPC17xx.h>                /* LPC17XX definitions         */
#include "lpc17xx_pinsel.h"
#include "lpc17xx_libcfg_default.h"
#include "GPIO_signals.h"
#include "System.h"


struct SW_UART_FIFO_STRUCT_TYPE{
  uint16_t		size;
	uint16_t		count;
	uint16_t		last;
	uint16_t		first;
};

extern void getline (char *line, int n);          /* input line               */

extern void Init (void);
extern void test(void);
extern void wait(uint32_t);

extern int8_t command;

extern void ADC_RUN(uint32_t *result);


#include "Global_header.h"
#include <stdio.h>
#include "lpc17xx_gpio.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_systick.h"
#include "lpc17xx_uart.h"
#include <LPC17XX.h>                       /* LPC17XX definitions         */
#include <system_LPC17xx.h>                /* LPC17XX definitions         */
#include "lpc17xx_pinsel.h"
#include "pt.h"														 /* PROTOTHREADS sources */
#include "lpc17xx_libcfg_default.h"
#include "math.h"
#include "GPIO_signals.h"
#include "System.h"
#include "lpc17xx_i2s.h"

struct SW_UART_FIFO_STRUCT_TYPE{
  uint16_t		size;
	uint16_t		count;
	uint16_t		last;
	uint16_t		first;
};

extern void getline (char *line, int n);          /* input line               */
extern struct pt Power_mgmt_pt, Calibration_pt;
extern uint8_t start_adc_bat_bit;
extern void InitProtothreads (void);
extern void Init (void);
extern void test(void);
extern void wait(uint32_t);
extern void AD9833_SetFreq(uint32_t freq);
extern void AD9833_SetPhase(uint16_t phase);
extern void AD9833_Stop(void);
extern void AD9833_Start(void);
extern uint32_t ADC_RUN(void);
extern PT_THREAD(Power_mgmt(struct pt *pt));
extern PT_THREAD(Calibration(struct pt *pt));

PT_THREAD(Starting(struct pt *pt));

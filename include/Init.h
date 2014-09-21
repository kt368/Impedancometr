#include "lpc17xx_gpio.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_systick.h"
#include <LPC17XX.h>                       /* LPC17XX definitions         */
#include <system_LPC17xx.h>                /* LPC17XX definitions         */
#include "lpc17xx_pinsel.h"
#include "pt.h"														 /* PROTOTHREADS sources */
#include "lpc17xx_libcfg_default.h"
#include "math.h"
#include "GPIO_signals.h"
#include "flash_nvol.h"
#include "System.h"
#include "lpc17xx_i2s.h"
#include "lpc17xx_clkpwr.h"

#define _ADC_CHANNEL        ADC_CHANNEL_3
#define LOW									0
#define HIGH								1

void Init_GPIO_Pins (void);
void Init (void);
void Init_ADC (void);
void Starting_init (void);
extern void SER_Init(uint32_t baudrate);

PT_THREAD(Starting(struct pt *pt));

I2S_CFG_Type ConfigStruct;
I2S_MODEConf_Type I2S_MODEConf;

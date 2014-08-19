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

#define _ADC_CHANNEL        ADC_CHANNEL_3

void Init_GPIO_Pins (void);
void Init (void);
void Init_ADC (void);
void Starting_init (void);
void InitProtothreads (void);

PT_THREAD(Starting(struct pt *pt));
PT_THREAD(Power_mgmt(struct pt *pt));
PT_THREAD(Calibration(struct pt *pt));

//Исправить - сделать общий хедер для АЦП
uint16_t ADC_RUN(LPC_ADC_TypeDef *ADCx, uint8_t channel, uint8_t n_Samples, uint8_t overs_bits);

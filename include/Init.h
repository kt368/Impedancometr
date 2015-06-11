#include "lpc17xx_gpio.h"

#include "lpc17xx_systick.h"
#include <LPC17XX.h>                       /* LPC17XX definitions         */
#include <system_LPC17xx.h>                /* LPC17XX definitions         */






#include "System.h"

#include "AD7793.h"				// AD7793 definitions.

#define _ADC_CHANNEL        ADC_CHANNEL_3
#define LOW									0
#define HIGH								1

void Init_GPIO_Pins (void);
void Init (void);

void Init_AD7793(void);
extern void SER_Init(uint32_t baudrate);

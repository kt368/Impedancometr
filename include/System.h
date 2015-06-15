#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"
#include "GPIO_signals.h"
#include <stdio.h>
#include "AD7793.h"				// AD7793 definitions.


//Definitions of UART commands
#define Calibrate	7
#define f					9
#define BADKEY		-1
#define	none			0


//Definitions of UART state
#define enter			1

#define UART    LPC_UART0
#define CNTLQ      0x11
#define CNTLS      0x13
#define DEL        0x7F
#define BACKSPACE  0x08
#define CR         0x0D
#define LF         0x0A

// Functions definitions

void Init_GPIO_Pins (void);
void Init (void);
void test (void);
void blink (void);

void ADC_RUN(uint32_t *result);


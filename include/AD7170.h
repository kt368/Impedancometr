#include "lpc17xx_gpio.h"
#include "lpc17xx_spi.h"
#include "lpc17xx_systick.h"
#include <LPC17XX.h>                       /* LPC17XX definitions         */
#include <system_LPC17xx.h>                /* LPC17XX definitions         */
#include "lpc17xx_pinsel.h"
#include "pt.h"														 /* PROTOTHREADS sources */
#include "lpc17xx_libcfg_default.h"
#include "math.h"

uint16_t AD7170_SPI_Read(uint8_t);
void AD7170_SPI_Read_Init(void);
/* --------------------------------- End Of File ------------------------------ */

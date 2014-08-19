#include "lpc17xx_gpio.h"
#include "lpc17xx_spi.h"
#include "lpc17xx_systick.h"
#include <LPC17XX.h>                       	/* LPC17XX definitions         */
#include <system_LPC17xx.h>                	/* LPC17XX definitions         */
#include "lpc17xx_pinsel.h"
#include "pt.h"															/* PROTOTHREADS sources */
#include "lpc17xx_libcfg_default.h"
#include "math.h"

#define AD9833_SPI_CLK_FREQ 		1000000			//SPI CLK frequency for communicate with AD9833

#define FSYNC_LOW								GPIO_ClearValue(0,1<<1)		//Set signal FSYNC to '0'
#define FSYNC_HIGH							GPIO_SetValue(0,1<<1)			//Set signal FSYNC to '1'

/*********************************************************************//**
 * AD9833 configuration parameter defines
 **********************************************************************/
/** Freqyency register length during configuring control bit */
#define COMPLETE_FREQ_WORD      ((uint16_t)(1<<13))
#define HALF_FREQ_WORD         	((uint16_t)(0))

/** Half part of rfequency register control bit */
#define HLB_LSB             		((uint16_t)(0))
#define HLB_MSB             		((uint16_t)(1<<12))

/** FREQ register select bit */
#define FREQ0										((uint16_t)(0))
#define FREQ1										((uint16_t)(1<<11))

/** PHASE register select bit */
#define PHASE0									((uint16_t)(0))
#define PHASE1									((uint16_t)(1<<10))

/** PHASE register select bit */
#define AD9833_RESET						((uint16_t)(1<<8))
#define AD9833_RESET_DISABLE		((uint16_t)(0))

/** SLEEP functions bits */
#define NO_PD										((uint16_t)(0))
#define DAC_PD									((uint16_t)(1<<6))
#define INTERNAL_CLK_PD					((uint16_t)(1<<7))
#define FULL_PD									((uint16_t)(3<<6))

/** Output signal form select bit */
#define SINUSOIDAL							((uint16_t)(0))
#define TRIANGLE								((uint16_t)(1<1))
#define SQUARE									((uint16_t)(1<5))

/** Square output signal div2 frequency bit */
#define DIV2										((uint16_t)(0))
#define NODIV2									((uint16_t)(1<3))

/** AD9833 registers adresses */
#define FREQ0_REG								((uint16_t)(1<<14))
#define FREQ1_REG								((uint16_t)(1<<15))
#define PHASE0_REG							((uint16_t)(3<<14))
#define PHASE1_REG							((uint16_t)(7<<13))

/* Public Functions ----------------------------------------------------------- */
/** @defgroup SPI_Public_Functions SPI Public Functions
 * @{
 */


void AD9833_Init(void);									// AD9833 init function
void AD9833_SPI_Init(void);							// Init SPI for work with AD9833
void AD9833_SetFreq(uint32_t freq);			// AD9833 frequensy select function
void AD9833_SetPhase(uint16_t phase);		// AD9833 phase select function
void AD9833_Stop(void);									// AD9833 stop generating function
void AD9833_Run(void);									// AD9833 start generating function


/**
 * @}
 */

/* --------------------------------- End Of File ------------------------------ */

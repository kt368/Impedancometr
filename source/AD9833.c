/**************************************************************************//**
 * @file     AD9833.c
 * @brief    AD9833 file
 * @note
 *
******************************************************************************/

#include "AD9833.h"
#include "lpc17xx_spi.h"
#include "lpc17xx_clkpwr.h"
//extern void CLKPWR_ConfigPPWR (uint32_t PPType, FunctionalState NewState);

SPI_CFG_Type SPI_GonfigStruct_temp;

	void AD9833_SPI_Init(void)
	{
		SPI_GetConfig(LPC_SPI, &SPI_GonfigStruct_temp);
		CLKPWR_ConfigPPWR (CLKPWR_PCONP_PCSPI, ENABLE);
		LPC_SPI->SPCR = SPI_CPHA_FIRST | SPI_CPOL_LO \
        | SPI_DATA_MSB_FIRST | SPI_DATABIT_16 \
        | SPI_MASTER_MODE | SPI_SPCR_BIT_EN;

		SPI_SetClock(LPC_SPI, AD9833_SPI_CLK_FREQ);
	}

	void AD9833_Start(void)
	{
		AD9833_SPI_Init();
		FSYNC_LOW;
		SPI_SendData(LPC_SPI, COMPLETE_FREQ_WORD | FREQ0 | PHASE0 | AD9833_RESET_DISABLE | NO_PD | SINUSOIDAL);
		while( SPI_CheckStatus( SPI_GetStatus(LPC_SPI), SPI_STAT_SPIF) == RESET );
		FSYNC_HIGH;
		// Reset SPI configs to heir previous condition (which have been before calling function AD9833_Init)
		SPI_Init(LPC_SPI, &SPI_GonfigStruct_temp);
	}
	
	void AD9833_Init(void)
	{
		AD9833_SPI_Init();
		FSYNC_LOW;
		GPIO_ClearValue(0,1<<30);
		SPI_SendData(LPC_SPI, COMPLETE_FREQ_WORD | FREQ0 | PHASE0 | AD9833_RESET | NO_PD | SINUSOIDAL);
		while( SPI_CheckStatus( SPI_GetStatus(LPC_SPI), SPI_STAT_SPIF) == RESET );
		FSYNC_HIGH;
		// Reset SPI configs to heir previous condition (which have been before calling function AD9833_Init)
		SPI_Init(LPC_SPI, &SPI_GonfigStruct_temp);
	}

	void AD9833_SetFreq(uint32_t freq)
	{
		volatile uint32_t FreqReg;
		volatile uint32_t temp;
		extern volatile uint32_t DdsFreq;
		
		DdsFreq = freq;
		FreqReg=(uint32_t)( ((uint64_t)(freq)) * 268435456/25000000 );
		AD9833_SPI_Init();
		FSYNC_LOW;
		
		//Sending command for transfering freq reg
		SPI_SendData(LPC_SPI, COMPLETE_FREQ_WORD | FREQ0 | PHASE0 | AD9833_RESET | NO_PD | SINUSOIDAL);
		while( SPI_CheckStatus( SPI_GetStatus(LPC_SPI), SPI_STAT_SPIF) == RESET );
		
		SPI_SendData(LPC_SPI, FREQ0_REG | (FreqReg & 0x3FFF));  // Sent 14LSB of frequency
		while( SPI_CheckStatus( SPI_GetStatus(LPC_SPI), SPI_STAT_SPIF) == RESET );
		
		SPI_SendData(LPC_SPI, FREQ0_REG | ( (FreqReg >> 14) & 0x3FFF));  // Sent 14MSB of frequency
		while( SPI_CheckStatus( SPI_GetStatus(LPC_SPI), SPI_STAT_SPIF) == RESET );
		
		AD9833_Start();
		
		FSYNC_HIGH;

		// Reset SPI configs to heir previous condition (which have been before calling function AD9833_SetFreq
		SPI_Init(LPC_SPI, &SPI_GonfigStruct_temp);
	}
	
	void AD9833_SetPhase(uint16_t phase)
	{
		AD9833_SPI_Init();
		FSYNC_LOW;
		SPI_SendData(LPC_SPI, PHASE0_REG | (phase & 0xFFF));  // Sent phase
		while( SPI_CheckStatus( SPI_GetStatus(LPC_SPI), SPI_STAT_SPIF) == RESET );
		AD9833_Start();
		FSYNC_HIGH;
		// Reset SPI configs to heir previous condition (which have been before calling function AD9833_SetPhase)
		SPI_Init(LPC_SPI, &SPI_GonfigStruct_temp);
	}
	
	void AD9833_Stop(void)
	{
		AD9833_Init();
	}

/* --------------------------------- End Of File ------------------------------ */

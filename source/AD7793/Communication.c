/***************************************************************************//**
 *   @file   Communication.c
 *   @brief  Implementation of Communication Driver.
 *   @author DBogdan (dragos.bogdan@analog.com)
********************************************************************************
 * Copyright 2012(c) Analog Devices, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Analog Devices, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *  - The use of this software may or may not infringe the patent rights
 *    of one or more patent holders.  This license does not release you
 *    from the requirement that you obtain separate licenses from these
 *    patent holders to use this software.
 *  - Use of the software either in source or binary form, must be run
 *    on or directly connected to an Analog Devices Inc. component.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ANALOG DEVICES BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, INTELLECTUAL PROPERTY RIGHTS, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
********************************************************************************
 *   SVN Revision: 500
*******************************************************************************/

/******************************************************************************/
/* Include Files                                                              */
/******************************************************************************/
#include "Communication.h"
extern unsigned char AD7793_Init(void);

/***************************************************************************//**
 * @brief Initializes the SPI communication peripheral.
 *
 * @param lsbFirst - Transfer format (0 or 1).
 *                   Example: 0x0 - MSB first.
 *                            0x1 - LSB first.
 * @param clockFreq - SPI clock frequency (Hz).
 *                    Example: 1000 - SPI clock frequency is 1 kHz.
 * @param clockPol - SPI clock polarity (0 or 1).
 *                   Example: 0x0 - idle state for SPI clock is low.
 *	                          0x1 - idle state for SPI clock is high.
 * @param clockPha - SPI clock phase (0 or 1).
 *                   Example: 0x0 - data is latched on the leading edge of SPI
 *                                  clock and data changes on trailing edge.
 *                            0x1 - data is latched on the trailing edge of SPI
 *                                  clock and data changes on the leading edge.
 *
 * @return 0 - Initialization failed, 1 - Initialization succeeded.
*******************************************************************************/
//unsigned char SPI_Init(unsigned char lsbFirst,
//                       unsigned long clockFreq,
//                       unsigned char clockPol,
//                       unsigned char clockPha)
//{
//	// Add your code here.
//	
//    return(1);
//}

/***************************************************************************//**
 * @brief Writes data to SPI.
 *
 * @param data - Write data buffer:
 *               - first byte is the chip select number;
 *               - from the second byte onwards are located data bytes to write.
 * @param bytesNumber - Number of bytes to write.
 *
 * @return Number of written bytes.
*******************************************************************************/
unsigned char SPI_Write(unsigned char* data,
                        unsigned char bytesNumber)
{
	uint8_t i;
	SpiInitForAD7793(LPC_SPI);
	
	if (data[0] == 1)
	{
		GPIO_ClearValue(1, 1<<29);
	}

	for (i=1;i<=bytesNumber;i++)
	{
		SPI_SendData(LPC_SPI,data[i]);
		while( SPI_CheckStatus( SPI_GetStatus(LPC_SPI), SPI_STAT_SPIF) == RESET );
	}
	
	if (data[0] == 1)
	{
		GPIO_SetValue(1, 1<<29);
	}
	return(bytesNumber);
}

/***************************************************************************//**
 * @brief Reads data from SPI.
 *
 * @param data - As an input parameter, data represents the write buffer:
 *               - first byte is the chip select number;
 *               - from the second byte onwards are located data bytes to write.
 *               As an output parameter, data represents the read buffer:
 *               - from the first byte onwards are located the read data bytes. 
 * @param bytesNumber - Number of bytes to write.
 *
 * @return Number of written bytes.
*******************************************************************************/

// ÏÐÎÂÅÐÈÒÜ ÏÐÈÌÅÍÅÍÈÅ ÔÓÍÊÖÈÈ SPI_ReadWrite
unsigned char SPI_Read(unsigned char* data,
                       unsigned char bytesNumber)
{
	
	uint8_t chipSelect    = data[0];
	uint8_t writeData[4]  = {0, 0, 0, 0};
	uint8_t readData[4]	= {0, 0, 0, 0};
	uint8_t byte          = 0;
  SpiInitForAD7793(LPC_SPI);
     for(byte = 0;byte < bytesNumber;byte ++)
    {
        writeData[byte] = data[byte + 1];
        data[byte + 1] = 0;
    }
    if(chipSelect == 1)
    {
        ADI_PART_CS_LOW;
    }

    for(byte = 0;byte < bytesNumber;byte ++)
    {
			SPI_SendData(LPC_SPI, writeData[byte]);
			while( SPI_CheckStatus( SPI_GetStatus(LPC_SPI), SPI_STAT_SPIF) == RESET );
			readData[byte] = SPI_ReceiveData(LPC_SPI);
    }
    if(chipSelect == 1)
    {
        ADI_PART_CS_HIGH;
    }
	for(byte = 0;byte < bytesNumber;byte ++)
    {
        data[byte] = readData[byte];
    }
		
	return(bytesNumber);
}

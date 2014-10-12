/*----------------------------------------------------------------------------
 * Name:    Serial.c
 * Purpose: Low Level Serial Routines
*/

#include "Serial.h"
#include "lpc17xx_clkpwr.h"
#include "AD9833.h"

extern uint32_t C;

int8_t command = none;
int8_t UART_STATE = none;

char cmdbuf [15];
int uart_rcv_len_cnt = 0;
char c;

uint8_t UART_pressed_enter = 0;

extern struct SW_UART_FIFO_STRUCT_TYPE SW_UART_FIFO_STRUCT;
extern uint8_t SW_UART_pop(void);

extern void led_on(void);
extern void led_off(void);

extern uint8_t bg_flag;
extern uint16_t f_min, f_max, n_F;
uint8_t StartADC=0,StartADC_mag=0,StartADC_ph=0;
uint32_t ADCnumber=32768;
uint8_t os=16;

extern uint8_t debug_mode, raw_data_mode;

extern uint_fast8_t UartWithFifo;

void SER_Init(uint32_t baudrate)
{
	UART_CFG_Type UART_ConfigStruct;
	UART_FIFO_CFG_Type UART_FIFOInitStruct;
	
	UART_ConfigStruct.Baud_rate = baudrate;
	UART_ConfigStruct.Parity = UART_PARITY_NONE;
	UART_ConfigStruct.Stopbits = UART_STOPBIT_1;
	UART_ConfigStruct.Databits = UART_DATABIT_8;
	
	UART_Init(LPC_UART0, &UART_ConfigStruct);							//Init UART0 with parameters, defined above

	UART_TxCmd(LPC_UART0, ENABLE);												//Enable transmission on UART TxD pin

	UART_FIFOConfigStructInit(&UART_FIFOInitStruct);			//Fills each UART_FIFOInitStruct member with its default value
	UART_FIFOConfig(LPC_UART0, &UART_FIFOInitStruct);			//Configure FIFO function on UART0 peripheral
	
	UART_IntConfig(LPC_UART0,UART_INTCFG_RBR,ENABLE);			//Enable UART0 RBR Interrupt
	NVIC_EnableIRQ(UART0_IRQn);														//Enable UART0 Interrupts
	
}

int SER_PutChar (int c) {

  while (!(UART->LSR & 0x20));
  UART->THR = c;

  return (c);
}

void UART0_IRQHandler(void)
{
	static char *line = &cmdbuf[0];
	static uint16_t frequency=1;
	float results[2];
	uint32_t temp;
	uint32_t U0IIR = 0;
	uint32_t baudrate;
	extern uint8_t CorrectIndexesOverride;
	extern uint8_t CorrectIndexesBrute;
	extern uint8_t EqualIndexes;
	extern uint16_t FirstOverrideIndex;
	extern uint16_t SecondOverrideIndex;
	char *EndFromStrtoul;
	
	U0IIR=UART_GetIntId(LPC_UART0);
	if ((U0IIR & 0xE) == 0x4)
	{
		c = UART_ReceiveByte(LPC_UART0);
    if (c == CR) c = LF;  /* read character                 */
		if (c != LF)
		{
			if (c == BACKSPACE  ||  c == DEL) {     /* process backspace              */
				if (uart_rcv_len_cnt != 0)  {
					uart_rcv_len_cnt--;                              /* decrement count                */
					line--;                             /* and line pointer               */
					putchar (BACKSPACE);                /* echo backspace                 */
					putchar (' ');
					putchar (BACKSPACE);
				}
			}
			else if (c != CNTLQ && c != CNTLS) {    /* ignore Control S/Q             */
				putchar (*line = c);                  /* echo and store character       */
				line++;                               /* increment line pointer         */
				uart_rcv_len_cnt++;                                /* and count                      */
			}
			if (uart_rcv_len_cnt == sizeof(cmdbuf))
			{
				printf("\nError.");
				printf("\nBuffer full.");
			}
		}
		else
		{
			line = &cmdbuf[0];
			UART_pressed_enter = 1;
			if (command == none)
			{
				if ( (strcmp(cmdbuf, "cal") == 0) && uart_rcv_len_cnt == 3)
				{
					command = Calibrate;
					printf("\n Entered into calibrating state. Type calibrating command.\n>");
					UART_pressed_enter = 0;
				}
				else if ( (strcmp(cmdbuf, "m") == 0) && uart_rcv_len_cnt == 1)
				{
					wait(1);
					Measure(results, frequency);
					printf("\n Magnitude of impedance is: ");
					printf("%.1f Ohm.", results[0]);
					printf("\n Phase of impedance is: ");
					printf("%.2f graduses.", results[1]*57.295779513);
					printf("\nType next command.\n>");
					UART_pressed_enter = 0;
				}
				else if (strncmp(cmdbuf, "f ", 2) == 0)
				{
					frequency = atoi(&(cmdbuf[2]));
					AD9833_SetFreq(frequency*1000);
					AD9833_Start();
					printf("\n New DDS frequency is %u kHz. ", frequency);
					printf("\n Type next command.\n>");
					UART_pressed_enter = 0;
				}
				else if (strncmp(cmdbuf, "fmin ", 5) == 0)
				{
					f_min = atoi(&(cmdbuf[5]));
					printf("\n f_min are %u kHz.", f_min);
					printf("\n Type next command.\n>");
					UART_pressed_enter = 0;
				}
				else if (strncmp(cmdbuf, "fmax ", 5) == 0)
				{
					f_max = atoi(&(cmdbuf[5]));
					printf("\n f_max are %u kHz.", f_max);
					printf("\n Type next command.\n>");
					UART_pressed_enter = 0;
				}
				else if (strncmp(cmdbuf, "nf ", 3) == 0)
				{
					n_F = atoi(&(cmdbuf[3]));
					printf("\n n_F are %u.", n_F);
					printf("\n Type next command.\n>");
					UART_pressed_enter = 0;
				}
				else if ((strncmp(cmdbuf, "bg", 2) == 0) && uart_rcv_len_cnt == 2)
				{
					bg_flag = 1;
					UART_pressed_enter = 0;
				}
				else if (strncmp(cmdbuf, "o ", 2) == 0)
				{
					os = atoi(&(cmdbuf[2]));
					ADCnumber = 1<<(os-1);
					printf("\n os are %u.", os);
					printf("\n Type next command.\n>");
					UART_pressed_enter = 0;
				}
				else if ( uart_rcv_len_cnt == 0)
				{
					NVIC_ClearPendingIRQ(I2S_IRQn);
					StartADC=1;
					StartADC_mag=1;
					StartADC_ph=1;
					UART_pressed_enter = 0;
				}
				else if (strncmp(cmdbuf, "g", 1) == 0)
				{
					temp=ADC_RUN(os);
					printf("\n Phase are %u.", (uint16_t)((temp & 0xffff0000) >> 16));
					printf("\n Magnitude are %u.", (uint16_t)(temp & 0xffff));
					printf("\n Type next command.\n>");
					UART_pressed_enter = 0;
				}
				else if (strncmp(cmdbuf, "d ", 2) == 0)
				{
					debug_mode = atoi(&(cmdbuf[2]));
					if (debug_mode == 0)
					{
						printf("\n Debug mode disabled.");
					}
					else
					{
						printf("\n Debug mode enabled.");
					}
					printf("\nType next command.\n>");
					UART_pressed_enter = 0;
				}
				else if (strncmp(cmdbuf, "raw ", 4) == 0)
				{
					raw_data_mode = atoi(&(cmdbuf[4]));
					if (raw_data_mode == 0)
					{
						printf("\n Raw data mode disabled.");
					}
					else
					{
						printf("\n Raw data mode enabled.");
					}
					printf("\nType next command.\n>");
					UART_pressed_enter = 0;
				}
				else if (strncmp(cmdbuf, "UartFifo ", 9) == 0)
				{
					UartWithFifo = atoi(&(cmdbuf[9]));
					if (UartWithFifo == 0)
					{
						printf("\n Software UART FIFO disabled.");
					}
					else
					{
						printf("\n Software UART FIFO enabled.");
					}
					printf("\nType next command.\n>");
					UART_pressed_enter = 0;
				}
				else if (strncmp(cmdbuf, "UART ", 5) == 0)
				{
					baudrate = atoi(&(cmdbuf[5]));
					SER_Init(baudrate);
					printf("\n UART baudrate are %u bps.", baudrate);
					printf("\nType next command.\n>");
					UART_pressed_enter = 0;
				}
				else if (strncmp(cmdbuf, "CIO ", 4) == 0)
				{
					CorrectIndexesOverride = atoi(&(cmdbuf[4]));
					if (CorrectIndexesOverride == 1)
					{
						printf("\nEnter correct override indexes:\n>");
						command = CorrectIndexesPutting;
					}
					else
					{
						printf("\n Index overriding disabled.");
						printf("\nType next command.\n>");
					}
					UART_pressed_enter = 0;
				}
				else if (strncmp(cmdbuf, "CIB ", 4) == 0)
				{
					CorrectIndexesBrute = atoi(&(cmdbuf[4]));
					if (CorrectIndexesBrute == 1)
					{
						printf("\n Correct calibrating curves brute forcing enabled.");
						printf("\nType next command.\n>");
					}
					else
					{
						printf("\n Correct calibrating curves brute force disabled.");
						printf("\nType next command.\n>");
					}
					UART_pressed_enter = 0;
				}
				else if (strncmp(cmdbuf, "EI ", 3) == 0)
				{
					EqualIndexes = atoi(&(cmdbuf[3]));
					if (EqualIndexes == 1)
					{
						printf("\n Equal calibrating indexes for mag and phase enabled.");
						printf("\nType next command.\n>");
					}
					else
					{
						printf("\n Equal calibrating indexes for mag and phase disabled.");
						printf("\nType next command.\n>");
					}
					UART_pressed_enter = 0;
				}
				else
				{
					printf("\nWrong command. Please type right command.\n");
					UART_pressed_enter = 0;
				}
				memset(cmdbuf,0,15);
				uart_rcv_len_cnt=0;
			}
			else if (command == CorrectIndexesPutting)
			{
				FirstOverrideIndex = strtoul(cmdbuf, &EndFromStrtoul, 10);
				SecondOverrideIndex = atoi(EndFromStrtoul);
				printf("\n FirstCorrectIndex = %u", FirstOverrideIndex);
				printf("\n SecondCorrectIndex = %u", SecondOverrideIndex);
				printf("\nType next command.\n>");
				UART_pressed_enter = 0;
				command = none;
				memset(cmdbuf,0,15);
				uart_rcv_len_cnt=0;
			}
		}
	}
	else if ((U0IIR & 0xE) == 0x2)
	{
		if (SW_UART_FIFO_STRUCT.count!=0)
		{
			LPC_UART0->THR = SW_UART_pop();
		}
		else
			UART_IntConfig(LPC_UART0,UART_INTCFG_THRE,DISABLE);			//Disable UART0 THRE Interrupt
	}
}

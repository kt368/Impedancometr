/*----------------------------------------------------------------------------
 * Name:    Serial.c
 * Purpose: Low Level Serial Routines
*/

#include "Serial.h"
#include "lpc17xx_clkpwr.h"

extern uint32_t C;

int8_t command = none;

char cmdbuf [15];
int uart_rcv_len_cnt = 0;
char c;

uint8_t UART_pressed_enter = 0;

extern struct SW_UART_FIFO_STRUCT_TYPE SW_UART_FIFO_STRUCT;
extern uint8_t SW_UART_pop(void);

extern uint8_t test_flag;

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
	uint32_t U0IIR = 0;
	uint32_t result[2];
	

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
				if (strncmp(cmdbuf, "g", 1) == 0)
				{
					ADC_RUN(result);
					printf("\n  Magnitude are %u.", result[0]);
					printf("\n  Phase are %u.", result[1]);
					printf("\n Type next command.\n>");
					UART_pressed_enter = 0;
				}
				else if ((strncmp(cmdbuf, "test", 4) == 0) && uart_rcv_len_cnt == 4)
				{
					test_flag = 1;
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

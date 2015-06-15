#include "main.h"   /* My big header definitions         */
#include "AD9833.h"
#include "lpc17xx_clkpwr.h"
#include "system.h"

extern struct SW_UART_FIFO_STRUCT_TYPE SW_UART_FIFO_STRUCT;

uint8_t test_flag = 0;

	int main (void) {
		
	char cmdbuf [15];
	
	uint16_t i;
	
	Init();
	
	printf("\f");
	printf("Started!");
	
	blink();
		
		
	AD8302_MAG;
	AD9833_SetFreq(100000);
	AD9833_SetPhase(0);
	AD9833_Start();
	
	printf ("\nType command.\n>");
	
  getline (&cmdbuf[0], sizeof (cmdbuf));   /* input command line            */
	
	while(1)
	{
		if (command == Calibrate)
		{
			
		}
		if (test_flag == 1)
		{
			AD7793_SetChannel(AD7793_CH_AIN1P_AIN1M);
			for (i = 0; i < 2000; i++)
				{
					printf("\n%lu", AD7793_SingleConversion());
				}
				printf("\nType next command.\n>");
				test_flag = 0;
		}
	}
}


// END OF FILE

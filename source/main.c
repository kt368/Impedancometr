#include "main.h"   /* My big header definitions         */
#include "AD9833.h"
#include "lpc17xx_clkpwr.h"

extern struct SW_UART_FIFO_STRUCT_TYPE SW_UART_FIFO_STRUCT;

uint8_t test_flag = 0;

	int main (void) {
		
	char cmdbuf [15];
	
	uint16_t i;
	
	Init();
	
	printf("\f");
	printf("Started!");
	
	blink();
	
	printf ("\nType command.\n>");
	
  getline (&cmdbuf[0], sizeof (cmdbuf));   /* input command line            */
	
	while(1)
	{
		if (test_flag == 1)
		{
			AD7793_SetChannel(AD7793_CH_AIN1P_AIN1M);
			for (i = 0; i < 5000; i++)
				{
					printf("\n%lu", AD7793_SingleConversion());
				}
				printf("\nType next command.\n>");
				test_flag = 0;
		}
	}
}


// END OF FILE

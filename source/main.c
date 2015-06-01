#include "main.h"   /* My big header definitions         */
#include "AD9833.h"
#include "lpc17xx_clkpwr.h"

// a unique identifier for the non-volatile variable
#define nonvol 1
#define m 1
#define s 0

extern struct SW_UART_FIFO_STRUCT_TYPE SW_UART_FIFO_STRUCT;

uint8_t bg_flag = 0;
uint16_t f_min = 10, f_max = 5000, n_F = 499;

extern uint16_t pMagCI[2];
extern uint16_t pPHCI[2];

	int main (void) {
		
	char cmdbuf [15];
	static uint16_t frequency=1;
	float results[2];
	extern uint8_t debug_mode;
	extern uint16_t mag;
	extern uint16_t ph;
	
	uint16_t i_F;
	
	InitProtothreads();
	
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
	
	test();
	
	while(1)
	{
		Calibration(&Calibration_pt);
		if (1)
		{
			
		}
		if (bg_flag == 1)
		{
				printf("\n\nImpedance vs frequency data:");
				printf("\nFreq\tMag\tPh\tADCmag\tADCph\tsdvig\tmagCI\t\tphCI");
				printf("\nkHz\tOhm\tdegree\n");
				for (i_F = 0; i_F < n_F; i_F++)
				{
					frequency = (f_min+(uint32_t)(f_max-f_min)*(uint32_t)i_F/n_F);
					AD9833_SetFreq(frequency*1000);
					AD9833_Start();
					wait(100);
					Measure(results, frequency);
					printf("\n%6u\t", frequency);
					printf("%6.1f\t", results[0]);
					printf("%6.2f\t", results[1]*57.295779513);
					printf("%6u\t", mag);
					printf("%6u\t", ph);
					printf("%4u", pMagCI[0]);
					printf("%4u", pMagCI[1]);
					printf("%4u", pPHCI[0]);
					printf("%4u", pPHCI[1]);
					//printf("%u\n", SW_UART_FIFO_STRUCT.count);
				}
				printf("\nType next command.\n>");
				bg_flag = 0;
		}
	}
}


// END OF FILE

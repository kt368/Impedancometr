#include "main.h"   /* My big header definitions         */
#include "lpc17xx_clkpwr.h"

extern struct SW_UART_FIFO_STRUCT_TYPE SW_UART_FIFO_STRUCT;

uint8_t test_flag = 0;
uint8_t bg_flag = 0;
uint16_t f_min = 10, f_max = 5000, n_F = 499;
uint16_t frequency = 100;

	int main (void) {
		
	char cmdbuf [15];
	
	uint16_t i;
	uint16_t i_F;
	uint16_t TMR0CNT;
		
	float results[2];
	extern uint8_t debug_mode;
	extern uint32_t mag;
	extern uint32_t ph;
	extern uint16_t pMagCI[2];
	extern uint16_t pPHCI[2];
		
	extern void Measure(float results[2], uint16_t freq);
	extern void LoadCalData(void);
	
	Init();
	LoadCalData();
	
	printf("\f");
	printf("Started!");
	
	blink();
		
		
	AD8302_MAG;
	AD9833_SetFreq(frequency);
	AD9833_SetPhase(0);
	AD9833_Start();
	
	printf ("\nType command.\n>");
	
  getline (&cmdbuf[0], sizeof (cmdbuf));   /* input command line            */
	
	while(1)
	{
		if (command == Calibrate)
		{
			Calibrating();
		}
		if (test_flag == 1)
		{
			AD7793_SetChannel(AD7793_CH_AIN1P_AIN1M);
			for (i = 0; i < 2008; i++)
				{
					printf("\n%lu", AD7793_SingleConversion());
				}
				printf("\nType next command.\n>");
				test_flag = 0;
		}
		if (bg_flag == 1)
		{
				printf("\n\nImpedance vs frequency data:");
				printf("\nFreq\tMag\t Ph\t ADCmag\t ADCph\t  magCI\t  phCI\t  time consumed");
				printf("\nkHz\tOhm\tdegree\t\t\t\t\t  ms\n");
				for (i_F = 0; i_F < n_F; i_F++)
				{
					frequency = (f_min+(uint32_t)(f_max-f_min)*(uint32_t)i_F/n_F);
					AD9833_SetFreq(frequency*1000);
					AD9833_Start();
					wait(1);
					TIM_ResetCounter(LPC_TIM0);
					Measure(results, frequency);
					TMR0CNT = LPC_TIM0->TC;
					printf("%-6u\t", frequency);
					printf("%6.1f\t", results[0]);
					printf("%6.2f\t", results[1]*57.295779513);
					printf("%7u\t", mag);
					printf("%7u\t", ph);
					printf("%4u", pMagCI[0]);
					printf("%4u", pMagCI[1]);
					printf("%4u", pPHCI[0]);
					printf("%4u", pPHCI[1]);
					printf("%5u\n", TMR0CNT);
					//printf("%u\n", SW_UART_FIFO_STRUCT.count);
				}
				printf("\nType next command.\n>");
				bg_flag = 0;
		}
	}
}


// END OF FILE

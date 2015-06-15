#include "System.h"

extern int8_t command;
volatile uint32_t DdsFreq;

	void ADC_RUN(uint32_t* result)
	{
		AD7793_SetChannel(AD7793_CH_AIN1P_AIN1M);
		result[0] = AD7793_SingleConversion();
		AD7793_SetChannel(AD7793_CH_AIN2P_AIN2M);
		result[1] = AD7793_SingleConversion();
	}
	
	void test(void)
	{
		uint8_t i=3;
		uint32_t d;
		
		while(i != 0)
		{
			i--;
			GPIO_SetValue (0, (1<<0));
			for (d = 0; d < 700000; d++);
			GPIO_ClearValue (0, (1<<0));
			for (d = 0; d < 700000; d++);
		}
	 
	}
	
	void blink(void)
	{
		uint32_t i;
		
		GPIO_SetValue (0, (1<<0));
		for (i = 0; i < 2000000; i++);
		GPIO_ClearValue (0, (1<<0));
	}






// END OF FILE

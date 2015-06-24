#include "System.h"

extern int8_t command;

volatile uint32_t DdsFreq;

uint32_t C=0;
uint32_t Rt = 10;
uint32_t Rb = 10;
uint32_t Rs = 0;
uint32_t Rp = 1106;
uint8_t debug_mode = 0;//, raw_data_mode = 0, FirstCalStart = 1;
uint16_t nZ_cal = 0;

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

/*********************************************************************//**
* @brief        	Задержка на указанное количество миллисекунд
	@param[t]				Задержка в мс
* @return       	none
**********************************************************************/
void wait(uint32_t t)
{
	t=t*9230;		//Подстройка для точной задержки. Зависит от тактовой частоты 
	while (t>0)
	{
		t--;
	}
}


// END OF FILE

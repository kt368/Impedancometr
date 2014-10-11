#include "main.h"   /* My big header definitions         */
#include "AD9833.h"
#include "lpc17xx_clkpwr.h"

// a unique identifier for the non-volatile variable
#define nonvol 1
#define m 1
#define s 0

static volatile uint8_t Counter;
extern struct SW_UART_FIFO_STRUCT_TYPE SW_UART_FIFO_STRUCT;

uint8_t bg_flag = 0;
uint16_t f_min = 10, f_max = 5000, n_F = 499;
 
uint8_t StartConvert=0;
uint32_t adc_buffer[100];
extern uint32_t i2s_fifo[8];
extern uint32_t ADCnumber;
extern uint8_t os;
extern uint8_t I2S_complete;
extern uint16_t pMagCI[2];
extern uint16_t pPHCI[2];

	int main (void) {
		
	PINSEL_CFG_Type PinCfg;
	static uint32_t adc_counter_mag=0, adc_counter_ph=0;
	static uint64_t sum=0, sum_ph;
	static float sko, sko_ph;
	uint8_t data_true = 0, mean_sko_mag=1, mean_sko_ph=1;
	uint32_t mean, mean_ph;
	
	volatile uint32_t temp_var=0, ff=10000;
	char cmdbuf [15];
	static uint16_t frequency=1;
	float results[2];
	volatile uint32_t temp, temp2;
	volatile float result=0;
	volatile float temp3;
	extern uint8_t StartADC,StartADC_mag,StartADC_ph;
	uint32_t i=0;
	uint_fast8_t j;
	extern uint_fast8_t sdvig;
	uint8_t sdvig_izmer_complete=0;
	extern uint8_t debug_mode;
	extern uint16_t mag;
	extern uint16_t ph;
	
	uint16_t i_F;
	volatile uint32_t i2s_fifo_buf[8];
	
		ADC_Init(LPC_ADC,200000);
	InitProtothreads();
	
	PinCfg.Funcnum = PINSEL_FUNC_1;
	PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	PinCfg.Pinnum = PINSEL_PIN_25;
	PinCfg.Portnum = PINSEL_PORT_0;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(0, (1<<25), 0); //Set P0.25 as input
	ADC_ChannelCmd(LPC_ADC,ADC_CHANNEL_2,ENABLE);
	LPC_ADC ->ADCR |= (0xf)<<8;
	
	Init();
	
	printf("\f");
	printf("Started!");
	
	test();
	AD8302_MAG;
	AD9833_SPI_Init();
	AD9833_SetFreq(100000);
	AD9833_SetPhase(0);
	AD9833_Start();
	test();
	

	printf ("\nType command.\n>");
	
  getline (&cmdbuf[0], sizeof (cmdbuf));   /* input command line            */
	
	while(1)
	{
		Calibration(&Calibration_pt);
		if ((StartADC==1)&& (I2S_complete==1))
		{
			if (sdvig_izmer_complete == 0)
			{
				//Измерение необходимого сдвига PCM сигнала - он почему-то плавает
				for (sdvig=3;sdvig<15;sdvig++)
				{
					for (j=0;j<64;j++)
					{
						while (I2S_complete == 0){};
						i2s_fifo_buf[0]=i2s_fifo[0];
						i2s_fifo_buf[1]=i2s_fifo[1];
						i2s_fifo_buf[2]=i2s_fifo[2];
						i2s_fifo_buf[3]=i2s_fifo[3];
						i2s_fifo_buf[4]=i2s_fifo[4];
						i2s_fifo_buf[5]=i2s_fifo[5];
						i2s_fifo_buf[6]=i2s_fifo[6];
						i2s_fifo_buf[7]=i2s_fifo[7];
						I2S_complete=0;
						for (i=0;i<8;i++)
						{
							if ( i2s_fifo_buf[i] & (( 1<<sdvig ) ))
							{
								temp3 ++;
							}
						}
					}
					if (temp3 == 0)
					{
						temp2 = sdvig;
					}
					temp3 = 0;
				}
				sdvig = temp2+1;
				sdvig_izmer_complete = 1;
			}
			i2s_fifo_buf[0]=i2s_fifo[0];
			i2s_fifo_buf[1]=i2s_fifo[1];
			i2s_fifo_buf[2]=i2s_fifo[2];
			i2s_fifo_buf[3]=i2s_fifo[3];
			i2s_fifo_buf[4]=i2s_fifo[4];
			i2s_fifo_buf[5]=i2s_fifo[5];
			i2s_fifo_buf[6]=i2s_fifo[6];
			i2s_fifo_buf[7]=i2s_fifo[7];
			I2S_complete=0;
			for (i=0;i<8;i++)
			{
				i2s_fifo_buf[i] = i2s_fifo_buf[i] >> sdvig;
				if (i2s_fifo_buf[i] != 0)
				{
					
					if (i2s_fifo_buf[i] >= 8388608)
					{
						i2s_fifo_buf[i] = i2s_fifo_buf[i] - 8388608;
						data_true = 1;
					}
					else
					{
						i2s_fifo_buf[i] = i2s_fifo_buf[i]+8388608;
						data_true = 1;
					}
					if ((i%2 == 0) && (debug_mode == 1))
					{
						printf("\n%u, ", i2s_fifo_buf[i]);
					}
					else if ((i%2 != 0) && (debug_mode == 1))
					{
						printf("%u", i2s_fifo_buf[i]);
					}
				}
				else
				{
					data_true = 0;
				}
				if (data_true == 1)
				{
					if ((i%2 != 0) && (StartADC_mag==1))
					{
						//модуль импеданса
						if (mean_sko_mag == m)
						{
							if (adc_counter_mag < ADCnumber)
							{
								sum += i2s_fifo_buf[i];
								adc_counter_mag ++;
							}
							else
							{
								mean = sum>>(os-1);
								sum=0;
								sko=0;
								mean_sko_mag = s;
								adc_counter_mag = 0;
								break;
							}
						}
						else
						{
							if (adc_counter_mag < ADCnumber)
							{
								sko+=(mean-i2s_fifo_buf[i])*(mean-i2s_fifo_buf[i]);
								adc_counter_mag ++;
							}
							else
							{
								sko = sqrtf(sko)/ADCnumber;
								adc_counter_mag = 0;
								mean_sko_mag = m;
								printf("\nMagnitude:");
								printf("\nMean %u",mean);
								printf("\nSKO %f",sko);
								printf("\nENOB %f\n", ((float)mean)/sko);
								sko = 0;
								StartADC_mag = 0;
							}
						}
					}
					else if (StartADC_ph==1)
					{
						//фаза импеданса
						if (mean_sko_ph == m)
						{
							if (adc_counter_ph < ADCnumber)
							{
								sum_ph += i2s_fifo_buf[i];
								adc_counter_ph ++;
							}
							else
							{
								mean_ph = sum_ph>>(os-1);
								sum_ph=0;
								sko_ph=0;
								mean_sko_ph = s;
								adc_counter_ph = 0;
								break;
							}
						}
						else
						{
							if (adc_counter_ph < ADCnumber)
							{
								sko_ph+=(mean_ph-i2s_fifo_buf[i])*(mean_ph-i2s_fifo_buf[i]);
								adc_counter_ph ++;
							}
							else
							{
								sko_ph = sqrtf(sko_ph)/ADCnumber;
								adc_counter_ph = 0;
								mean_sko_ph = m;
								printf("\nPhase:");
								printf("\nMean %u",mean_ph);
								printf("\nSKO %f",sko_ph);
								printf("\nENOB %f\n", ((float)mean_ph)/sko_ph);
								sko_ph = 0;
								StartADC_ph= 0;
							}
						}
						
					}
					if ((StartADC_mag==0) && (StartADC_ph==0))
					{
						StartADC = 0;
					}
				}
			}
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
					printf("%2u", sdvig);
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

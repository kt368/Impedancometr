#include "System.h"
#include "Flash_cal_adresses.h"

mag_ph_calc_calibr_struct_t MagPhcT_st;

// Calibration frequency list in kHz
uint32_t cal_freq_list[nF_cal] = { 10, 15, 20, 30, 50, 75, 100, 150, 200, 300, 400, 500, 700, 1000, 1400, 1900,\
																		2500, 3200, 4000, 5000 };
																	 
uint32_t C=0;
uint32_t Rt = 10;
uint32_t Rb = 10;
uint32_t Rs = 0;
uint32_t Rp = 1106;
struct IndZwithZ_str * pIndZwithZ;
extern void LoadCalData(void);
extern uint8_t os;
uint8_t I2S_complete;
volatile uint32_t DdsFreq;
uint8_t debug_mode = 0, FirstCalStart = 1;
uint16_t nZ_cal = 0;
uint_fast8_t sdvig;

struct CalData_struct* CalData;
struct CalData_struct* CalDataCalibr;
																		
uint8_t pCorrectIndexes[2] = {0, 0};

/*********************************************************************//**
* @brief        Run ADC conversion on selected chanel several times and return average value
* @param[in]    ADCx pointer to LPC_ADC_TypeDef, should be: LPC_ADC
* @param[in]    channel: channel number, should be 0..7
* @param[in]    n_Samples: requested quantity of ADC samples. Should be <= 16. N=2^n_Samples.
* @return       Average measured value
**********************************************************************/

	uint16_t ADC_int_RUN(LPC_ADC_TypeDef *ADCx, ADC_CHANNEL_SELECTION channel, uint8_t n_Samples, uint8_t overs_bits)
	{
		uint32_t adc_value=0;
		uint16_t counter, temp2;
		temp2=pow(2,n_Samples-1);
		CHECK_PARAM(PARAM_ADCx(ADCx));
    CHECK_PARAM(PARAM_ADC_CHANNEL_SELECTION(channel));
		for (counter=0;counter<temp2;counter++)
		{
			ADC_StartCmd(LPC_ADC,ADC_START_NOW);
			while( ((*(uint32_t *) ((&ADCx->ADDR0) + channel)) & ((0x1u)<<31)) == 0 );
			adc_value += (*(uint32_t *) ((&ADCx->ADDR0) + channel)) >> 4;
			LPC_ADC -> ADCR &= ~((0x1)<<24);
		}
		return (uint16_t)(adc_value>>(n_Samples-1-overs_bits));
	}
	
	uint32_t ADC_RUN(uint8_t n_Samples)
	{
		volatile uint32_t i2s_fifo_buf[8];
		extern uint32_t i2s_fifo[8];
		extern uint32_t ADCnumber;
		uint_fast8_t i, j, data_true, ADCInProgress=1;
		uint64_t SumAbs=0,SumPh=0;
		uint32_t MeanAbs, MeanPh;
		uint32_t temp = 0, temp2 = 0, temp3 = 0, temp4, temp5, adc_abs_counter=0, adc_ph_counter=0;
		
		//��������� ������������ ������ PCM ������� - �� ������-�� �������
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
		if (debug_mode == 1)
		{
			printf("\n%u", sdvig);
		}
		temp2=pow(2,n_Samples-1);
		
		while(ADCInProgress == 1)
		{
			if (I2S_complete==1)
			{
				i2s_fifo_buf[0]=i2s_fifo[0];
				i2s_fifo_buf[1]=i2s_fifo[1];
				i2s_fifo_buf[2]=i2s_fifo[2];
				i2s_fifo_buf[3]=i2s_fifo[3];
				i2s_fifo_buf[4]=i2s_fifo[4];
				i2s_fifo_buf[5]=i2s_fifo[5];
				i2s_fifo_buf[6]=i2s_fifo[6];
				i2s_fifo_buf[7]=i2s_fifo[7];
				I2S_complete=0;
				temp4 = i2s_fifo_buf[0];
				temp5 = i2s_fifo_buf[1];
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
					}
					else
					{
						data_true = 0;
					}
					
					if (data_true == 1)
					{
						if ( (adc_abs_counter < temp2) || (adc_ph_counter < temp2) )
						{
							if (i%2 != 0) //������ ���������
							{
								if (adc_abs_counter < temp2)
								{
									SumAbs += i2s_fifo_buf[i];
									adc_abs_counter++;
								}
							}
							else if (adc_ph_counter < temp2)
							{
								SumPh += i2s_fifo_buf[i];
								adc_ph_counter++;
							}
						}
						else
						{
							//����� �� 8 - ������� �� 24-� ������ ������ � 16-� ������
							MeanAbs = (SumAbs>>(n_Samples-1)) >> 8;
							MeanPh = (SumPh>>(n_Samples-1)) >> 8;
							SumAbs = 0;
							SumPh = 0;
							adc_abs_counter = 0;
							adc_ph_counter = 0;
							ADCInProgress = 0;
							if (debug_mode == 1)
							{
								printf("\nMeanAbs = %u", MeanAbs);
								printf("\nMeanPh = %u", MeanPh);
							}
							break;
						}
					}
				}
			}
		}
		return ( ( ((uint16_t)MeanAbs) & 0xffff ) | ( ((uint16_t)MeanPh) & 0xffff ) << 16 );
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

	void led_on(void)
	{
		GPIO_SetValue (0, (1<<0));
	}
	
	void led_off(void)
		{
			GPIO_ClearValue (0, (1<<0));
		}
	
	void V12SW_ON2(void)
	{
		uint16_t counter = 1000;
		volatile uint16_t wait_counter;
		volatile uint16_t internal_cycle_counter=0;
		while (counter>0)
		{
			GPIO_SetValue (1, (1<<22));
			GPIO_ClearValue (1, (1<<22));
			wait_counter=0;
			while (wait_counter<counter + 100)
			{
				wait_counter+=1000/counter;
			}
			internal_cycle_counter++;
			if (internal_cycle_counter>3)
			{
				internal_cycle_counter=0;
				counter--;
			}
			//if (counter == 80) break;
		}
		GPIO_SetValue (1, (1<<22));
	}
	
	
	/*********************************************************************//**
* @brief        Calculate magnitude and phase of calculated impedance from values, which were
								inputted throught UART
* @param[freq]	Current frequency, on which impedance must calculated, in Herz
* @return       none
**********************************************************************/
	
	void CalibrateMagPhaseCalcTheoretic(mag_ph_calc_calibr_struct_t *st)
	{
		float	temp, temp2;
		float pi = 3.1415926;
		led_on();
		temp = 2 * pi * st->freq * Rp * Rp * C * 0.000000000001;
		temp2 = 1 + temp * 2 * pi * st->freq * C * 0.000000000001;
		st->mag = hypotf(Rp / temp2 + Rs, temp / temp2);
		st->ph = - atan((temp / temp2) / (Rp / temp2 + Rs));
		led_off();
	}
	
/*********************************************************************//**
* @brief        	Generating logarithmically spaced numbers
* @param[base]		logarithmic base. Q15 format
	@param[n_bins]	Number of required data bins
	@param[min]			First bin value.	Q8 format
	@param[max]			Last bin value.	Q8 format
	@param[array]		Pinter to Data array, into which data wil be stored. Q8 format
* @return       	none
**********************************************************************/

	void logspace(uint32_t base, uint8_t n_bins, uint32_t min, uint32_t max, uint32_t array[])
	{
		uint64_t power;				//Q31 format
		uint64_t min_power;		//Q31 format
		uint64_t max_power;		//Q31 format
		uint64_t power_step;	//Q31 format
		
		base = base * 32768;
		
		min_power = (uint64_t) (2147483648LL * (log10f (min) - 2.40824) / (log10f(base) - 4.51545));
		max_power = (uint64_t) (2147483648LL * (log10f (max) - 2.40824) / (log10f(base) - 4.51545));
		power_step = (uint64_t) ((float)(max_power-min_power)/(n_bins-1));
		
		for (power = max_power; n_bins>0; power -= power_step)
		{
			array[n_bins-1] = (uint32_t)(pow(((float)base)/32768, ((float)power)/2147483648LL) * 256);
			n_bins--;
		}
	}


PT_THREAD(Calibration(struct pt *pt))
{
	uint8_t freq_counter;
	
	uint16_t mag_x_1;
	uint16_t ph_x_1;
	uint16_t mag_x_2;
	uint16_t ph_x_2;
	uint32_t temp_adc;
	
	float mag_y_1;
	float ph_y_1;
	float mag_y_2;
	float ph_y_2;
	uint8_t temp, temp2, temp3;
	
//declaration of external struct
	extern mag_ph_calc_calibr_struct_t MagPhcT_st;
	//declaration of pointer to external struct
	static mag_ph_calc_calibr_struct_t *pCalibrateMagPhaseCalcTheoretic_st=&MagPhcT_st;
	//declaration of external function, which receibe pointer to external struct
	extern void CalibrateMagPhaseCalcTheoretic(mag_ph_calc_calibr_struct_t *st);
	
	extern uint32_t cal_freq_list[];
	extern int8_t command;
	
	extern struct pt Calibration_pt;
	extern char cmdbuf [15];
	extern int uart_rcv_len_cnt;
	extern char c;
	extern uint8_t UART_pressed_enter;
	static uint8_t CalZCounter;
	struct PrepareStructForZcal_str PrepareStructForZcal;
	static uint8_t AllowSaveFlag=0;
	
	PT_BEGIN(pt);
	PT_WAIT_UNTIL(pt, (command == Calibrate));
	if (UART_pressed_enter)
	{
		UART_pressed_enter = 0;
		if ((strcmp(cmdbuf, "cap") == 0) && (uart_rcv_len_cnt == 3))
		{
			printf("\nEnter value of calibrating capacitor (which are serial with calibrating resistor and is located between electrodes):\n>");
			UART_pressed_enter = 0;
			PT_WAIT_UNTIL(pt, UART_pressed_enter);
			UART_pressed_enter = 0;
			sscanf(cmdbuf, "%u", &C);
			printf ("\n\rCapacitor value is %d pF.", C);
			printf("\nType next calibrating command.\n>");
		}
		else if ( atoi(cmdbuf) != 0 )
		{
			UART_pressed_enter = 0;
			sscanf(cmdbuf, "%u", &C);
			printf ("\n\rCapacitor value is %d pF.", C);
			printf("\nType next calibrating command.\n>");
		}
		else if ((strcmp(cmdbuf, "top") == 0) && uart_rcv_len_cnt == 3)
		{
			printf("\nEnter top calibrating resistor (which is located in the first electrode):\n>");
			UART_pressed_enter = 0;
			PT_WAIT_UNTIL(pt, UART_pressed_enter);
			UART_pressed_enter = 0;
			sscanf(cmdbuf, "%u", &Rt);
			printf ("\nTop resistor value is %d Ohm.", Rt);
			printf("\nType next calibrating command.\n>");
		}
		else if ((strcmp(cmdbuf, "bot") == 0) && uart_rcv_len_cnt == 3)
		{
			printf("\nEnter bottom calibrating resistor (which is located in the second electrode):\n>");
			UART_pressed_enter = 0;
			PT_WAIT_UNTIL(pt, UART_pressed_enter);
			UART_pressed_enter = 0;
			sscanf(cmdbuf, "%u", &Rb);
			printf ("\nBottom resistor value is %d Ohm.", Rb);
			printf("\nType next calibrating command.\n>");
		}
		else if ((strcmp(cmdbuf, "par") == 0) && uart_rcv_len_cnt == 3)
		{
			printf("\nEnter parralel calibrating resistor (which are parralel with calibrating capacitor and is located between electrodes):\n>");
			UART_pressed_enter = 0;
			PT_WAIT_UNTIL(pt, UART_pressed_enter);
			UART_pressed_enter = 0;
			sscanf(cmdbuf, "%u", &Rp);
			printf ("\nParralel resistor value is %d Ohm.", Rp);
			printf("\nType next calibrating command.\n>");
		}
		else if ((strcmp(cmdbuf, "ser") == 0) && uart_rcv_len_cnt == 3)
		{
			printf("\nEnter serial calibrating resistor (which are serial with calibrating capacitor and is located between electrodes):\n>");
			UART_pressed_enter = 0;
			PT_WAIT_UNTIL(pt, UART_pressed_enter);
			UART_pressed_enter = 0;
			sscanf(cmdbuf, "%u", &Rs);
			printf ("\nSerial resistor value is %d Ohm.", Rs);
			printf("\nType next calibrating command.\n>");
		}
		//else if ((strcmp(cmdbuf, "go") == 0) && uart_rcv_len_cnt == 2)
		else if ( uart_rcv_len_cnt == 0)
		{
				UART_pressed_enter = 0;
				if (CalZCounter == 0)
				{
					CalDataCalibr = malloc(sizeof(struct CalData_struct));
					CalDataCalibr->Zarray = malloc(nF_cal*sizeof(Zarray_t));
					CalDataCalibr->PHarray = malloc(nF_cal*sizeof(PHarray_t));
				}
				PrepareStructForZcal = GetPrepareStructForZcal();
				if (PrepareStructForZcal.RamSize == 0)
				{
					printf ("\nCapacitor value too big. Connect lower capacitance.\n>");
					wait(100);//wait 10 ms
					AllowSaveFlag = 0;
				}
				else
				{
					printf("\n%u",CalZCounter);
					// ��������� �� ���� ������ ���� ������� �������� ���������� � �� ���� ������� �������������
					// ������ ���������� ��������
					if (AllowSaveFlag == 1)
					{
						SaveCalData(CalDataCalibr, nonstop_saving);
					}
					CalDataCalibr->nFmin = PrepareStructForZcal.iFmin;
					CalDataCalibr->nFmax = PrepareStructForZcal.iFmax;
					//Lowest frequency
					pCalibrateMagPhaseCalcTheoretic_st->freq = cal_freq_list[PrepareStructForZcal.iFmin] * 1024;
					temp_adc = DdsFreq;
					AD9833_SetFreq(pCalibrateMagPhaseCalcTheoretic_st->freq);
					/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/wait((float)10 * temp_adc / (pCalibrateMagPhaseCalcTheoretic_st->freq)/1000);
					CalibrateMagPhaseCalcTheoretic(pCalibrateMagPhaseCalcTheoretic_st);
					mag_y_1 = pCalibrateMagPhaseCalcTheoretic_st->mag;
					ph_y_1 = pCalibrateMagPhaseCalcTheoretic_st->ph;
					temp_adc = ADC_RUN(os);
					ph_x_1 = (uint16_t)((temp_adc & 0xffff0000) >> 16);
					mag_x_1 = (uint16_t)(temp_adc & 0xffff);

					CalDataCalibr->gZmax = mag_x_1; //����������� ������������� �������� ������ ���������
					//printf("\n%4u kHz %4u, %u;", cal_freq_list[PrepareStructForZcal.iFmin], mag_x_1, ph_x_1);
					
					//freq_counter - ������ ������ ������� ���������� �������
					for (freq_counter = PrepareStructForZcal.iFmin; freq_counter < PrepareStructForZcal.iFmax; freq_counter ++)
					{
						//Lower frequency
						if (freq_counter != PrepareStructForZcal.iFmin)
						{
							mag_y_1 = mag_y_2;
							ph_y_1 = ph_y_2;
							mag_x_1 = mag_x_2;
							ph_x_1 = ph_x_2;
						}
						//Highest frequency
						pCalibrateMagPhaseCalcTheoretic_st->freq = cal_freq_list[freq_counter+1] * 1000;
						AD9833_SetFreq(pCalibrateMagPhaseCalcTheoretic_st->freq);
						wait(10);//wait 10 ms
						CalibrateMagPhaseCalcTheoretic(pCalibrateMagPhaseCalcTheoretic_st);
						mag_y_2 = pCalibrateMagPhaseCalcTheoretic_st->mag;
						ph_y_2 = pCalibrateMagPhaseCalcTheoretic_st->ph;
						temp_adc = ADC_RUN(os);
						ph_x_2 = (uint16_t)((temp_adc & 0xffff0000) >> 16);
						mag_x_2 = (uint16_t)(temp_adc & 0xffff);
						printf("\n%4u %4u %u", cal_freq_list[freq_counter+1], mag_x_2, ph_x_2);
//						printf("\n%4u kHz %4u, %u;", cal_freq_list[freq_counter+1], mag_x_2, ph_x_2);
						CalDataCalibr->Zarray[freq_counter-PrepareStructForZcal.iFmin].Zmin = mag_x_2;
						CalDataCalibr->PHarray[freq_counter-PrepareStructForZcal.iFmin].PHmin = ph_x_1;
						
						if (abs((int32_t)mag_x_2 - (int32_t)mag_x_1)<= 1)
							// ���� ���������� �������� ������ ��������� ����� �� ����������
						{
							CalDataCalibr->Zarray[freq_counter-CalDataCalibr->nFmin].k = 0;
							CalDataCalibr->Zarray[freq_counter-CalDataCalibr->nFmin].b = (mag_y_1 + mag_y_2) / 2;
						}
						else
							// ������ ��������� ���������, ������ ��� ����� ������
						{
							CalDataCalibr->Zarray[freq_counter-CalDataCalibr->nFmin].k = (mag_y_2-mag_y_1) / (mag_x_2-mag_x_1);
							CalDataCalibr->Zarray[freq_counter-CalDataCalibr->nFmin].b = mag_y_1-mag_x_1 * CalDataCalibr->Zarray[freq_counter-CalDataCalibr->nFmin].k;
						}
						if ((abs((int32_t)ph_x_2 - (int32_t)ph_x_1)<= 1))
							// ���� ���������� �������� ���� ��������� ����� �� ����������
						{
							CalDataCalibr->PHarray[freq_counter-CalDataCalibr->nFmin].k = 0;
							CalDataCalibr->PHarray[freq_counter-CalDataCalibr->nFmin].b = (ph_y_1 + ph_y_2) / 2;
						}
						else
							// ���� ��������� ���������, ������ ��� ����� ������
						{
							CalDataCalibr->PHarray[freq_counter-CalDataCalibr->nFmin].k = (ph_y_2-ph_y_1) / (ph_x_2-ph_x_1);
							CalDataCalibr->PHarray[freq_counter-CalDataCalibr->nFmin].b = ph_y_1-ph_x_1 * CalDataCalibr->PHarray[freq_counter-CalDataCalibr->nFmin].k;
						}
						
					}
					//����������� ������������ �������� ������ ���������
					CalDataCalibr->gZmin = 65535;
					for (temp=0; temp < (CalDataCalibr->nFmax - CalDataCalibr->nFmin); temp++)
					{
						if (CalDataCalibr->Zarray[temp].Zmin < CalDataCalibr->gZmin)
						{
							CalDataCalibr->gZmin = CalDataCalibr->Zarray[temp].Zmin;
						}
					}
					CalZCounter++;	//Increment calibrating impedances counter
					
					printf("\ngZmin  = %10u", CalDataCalibr->gZmin);
					//wait(50);
					printf("\ngZmax  = %10u", CalDataCalibr->gZmax);
					//wait(50);
					printf("\ngPHmin = %10u", CalDataCalibr->gPHmin);
					//wait(50);
					printf("\ngPHmax = %10u", CalDataCalibr->gPHmax);
					//wait(50);
					printf("\nnFmin  = %10u", CalDataCalibr->nFmin);
					//wait(50);
					printf("\nnFmax  = %10u", CalDataCalibr->nFmax);
					//wait(50);
					printf("\nZarray:");
					for (temp2 = 0; temp2 < (CalDataCalibr->nFmax - CalDataCalibr->nFmin); temp2++)
					{
						//wait(100);
						printf("\nk, b, Zmin = %10.5f, %10.5f, %10u", CalDataCalibr->Zarray[temp2].k, CalDataCalibr->Zarray[temp2].b, CalDataCalibr->Zarray[temp2].Zmin);
					}
					printf("\nPHarray:");
					for (temp2 = 0; temp2 < (CalDataCalibr->nFmax - CalDataCalibr->nFmin); temp2++)
					{
						//wait(100);
						printf("\nk * 10^5, b, PHmin = %10.5f, %10.5f, %10u", CalDataCalibr->PHarray[temp2].k*100000, CalDataCalibr->PHarray[temp2].b, CalDataCalibr->PHarray[temp2].PHmin);
					}
					//wait(100);
					
					if (CalZCounter == nZ_cal_max)
					{
						SaveCalData(CalDataCalibr, stop_saving);
						free(CalDataCalibr->Zarray);
						free(CalDataCalibr->PHarray);
						free(CalDataCalibr);
						printf("\nCalibration complete. Type next command.\n>");
						LoadCalData();
						command = none;
					}
					else
					{
						printf("\nCalibration on this calibration impedance complete. Type next calibrating command.\n>");
					}
					AllowSaveFlag = 1;
				}
		}
		else if ((strcmp(cmdbuf, "stop") == 0) && uart_rcv_len_cnt == 4)
		{
			if (CalZCounter != 0)
			{
				SaveCalData(CalDataCalibr, stop_saving);
				free(CalDataCalibr->Zarray);
				free(CalDataCalibr->PHarray);
				free(CalDataCalibr);
				printf("\nCalibration coefficients were saved into flash. Type next command.\n>");
				LoadCalData();
				command = none;
			}
			else
			{
				printf("\n Exiting from calibrating state...\n Nothing were stored. Type next command.\n>");
			}
			command = none;
		}
		else if ((strcmp(cmdbuf, "load") == 0) && uart_rcv_len_cnt == 4)
		{
			LoadCalData();
			if (debug_mode == 1)
			{
			for (temp = 0; temp < nZ_cal; temp++)
				{
					printf("\n%u", temp);
					//wait(5);
					printf("\ngZmin  = %10u", CalData[temp].gZmin);
					//wait(5);
					printf("\ngZmax  = %10u", CalData[temp].gZmax);
					//wait(5);
					printf("\ngPHmin = %10u", CalData[temp].gPHmin);
					//wait(5);
					printf("\ngPHmax = %10u", CalData[temp].gPHmax);
					//wait(5);
					printf("\nnFmin  = %10u", CalData[temp].nFmin);
					//wait(5);
					printf("\nnFmax  = %10u", CalData[temp].nFmax);
					//wait(5);
					printf("\nZarray:");
					for (temp2 = 0; temp2 < (CalData[temp].nFmax - CalData[temp].nFmin); temp2++)
					{
						//wait(10);
						printf("\nk, b, Zmin = %10.5f, %10.5f, %10u", CalData[temp].Zarray[temp2].k, CalData[temp].Zarray[temp2].b, CalData[temp].Zarray[temp2].Zmin);
					}
					printf("\nPHarray:");
					for (temp2 = 0; temp2 < (CalData[temp].nFmax - CalData[temp].nFmin); temp2++)
					{
						//wait(10);
						printf("\nk * 10^5, b, PHmin = %10.5f, %10.5f, %10u", CalData[temp].PHarray[temp2].k*100000, CalData[temp].PHarray[temp2].b, CalData[temp].PHarray[temp2].PHmin);
					}
					//wait(10);
				}
			}
			printf("\nCalibration data loaded from flash. Type next command.\n>");
			command = none;
		}
		else if ((strcmp(cmdbuf, "clear") == 0) && uart_rcv_len_cnt == 5)
		{
			CalZCounter--;
			AllowSaveFlag = 0;
			printf("\nLast impedance calibration data deleted. Type next command.\n>");
		}
		else
		{
			printf("\nWrong command. Please type calibrating command.\n>");
			UART_pressed_enter = 0;
		}
		memset(cmdbuf,0,15);
		uart_rcv_len_cnt=0;
		UART_pressed_enter = 0;
	}
  PT_END(pt);
}

/*********************************************************************//**
* @brief        	���������� ������ ������������� ��������, ������� �������� ����������� �������������
									��� ���������� �� �������, ������ ������� �������� �������.
	@param[freq_index]	������ �������. ������� ����� array cal_freq_list[������ �������]
* @return       	������ ������������� ��������, ������� �������� ������������ �������� ��� ������ �������.
**********************************************************************/
uint8_t Calc_iZ_for_Min_Z_OnCur_iF (uint8_t freq_index)
{
	uint8_t zcounter, z_index = 0;
	float min_Z, maybe_min_Z;
	//
	min_Z = CalData[0].gZmax;
	for (zcounter = 0; zcounter < nZ_cal; zcounter++)
	{
		if (freq_index >= CalData[zcounter].nFmax)
			maybe_min_Z = CalData[zcounter].gZmin;
		else if (freq_index <= CalData[zcounter].nFmin)
			maybe_min_Z = CalData[zcounter].gZmax;
		else
		{
			maybe_min_Z = CalData[zcounter].Zarray[freq_index - CalData[zcounter].nFmin-1].Zmin;
		}
		if (min_Z > maybe_min_Z)
		{
			z_index = zcounter;
			min_Z = maybe_min_Z;
		}
	}
	return z_index;
}

/*********************************************************************//**
* @brief        	���������� ������ ������������� ��������, ������� �������� ������������ ��������
									��� ���������� �� �������, ������ ������� �������� �������.
	@param[freq_index]	������ �������. ������� ����� array cal_freq_list[������ �������]
* @return       	������ ������������� ��������, ������� �������� ������������ �������� ��� ������ �������.
**********************************************************************/
uint8_t Calc_iZ_for_Max_Z_OnCur_iF (uint8_t freq_index)
{
	uint8_t zcounter, z_index = 0;
	float max_Z, maybe_max_Z;
	max_Z = CalData[0].gZmin;
	for (zcounter = 0; zcounter < nZ_cal; zcounter++)
	{
		if (freq_index >= CalData[zcounter].nFmax)
			maybe_max_Z = CalData[zcounter].gZmin;
		else if (freq_index <= CalData[zcounter].nFmin)
			maybe_max_Z = CalData[zcounter].gZmax;
		else
		{
			maybe_max_Z = CalData[zcounter].Zarray[freq_index - CalData[zcounter].nFmin-1].Zmin;
		}
		
		if (maybe_max_Z > max_Z)
		{
			z_index = zcounter;
			max_Z = maybe_max_Z;
		}
	}
	return z_index;
}

/*********************************************************************//**
* @brief        	������� ��������� ������������� ������ �������� �������� ������ ��������� ������������� ��������
									� �� ��������� �� ������ �������. ���������� ������� �������������� �� �������� �������� ���������
									���������������� �������� �������������. ���� ��������� ������� �� ������������� ������� �� ������
									������������� ������ (CalData_str.freq[]), ��������, �� �������� ����������� ����������
									�������������� �� ���� �������� ������������� �� �������� �������� (��� ����������� ������).
	@param[pIndZwithZ]		��������� �� ��������� ���� IndZwithZ_str. � �������� ������ ������� ���������� ������ ���
												CalData_str.nZ �������� ������� ���� � ��������� ����������� �� ������ ���� ��������.
	@param[freq]		������� � ���
* @return       	none
**********************************************************************/
void SortZIndOnCurF (struct IndZwith_uint_Z_str * pIndZwith_uint_Z, uint16_t freq)
{
	uint8_t freq_counter=0, iZ;
	
	//����������, � ����� �� ��������, ������������ �� �������� ������� cal_freq_list, �������� freq
	while (cal_freq_list[freq_counter] < freq)
	{
		freq_counter++;
	}
	
	if (cal_freq_list[freq_counter] == freq	)	//���� ������������� ������� - ���� �� ������, �� ������� �������������
																						//����������
	{
		for (iZ=0; iZ<nZ_cal; iZ++)
		{
			pIndZwith_uint_Z[iZ].Z = GetCalZOn_iZ_iF (iZ, freq_counter);
			pIndZwith_uint_Z[iZ].iZ = iZ;
		}
	}
	else	//���� �� ������������� ������� �� ������������� ����������
	{
		freq_counter--;	//������ ����� ������� ��������� freq_counter, ���� �� ����� ��������� �� �������, ������� ����,
										//��� freq
		for (iZ=0; iZ<nZ_cal; iZ++)
		{
			pIndZwith_uint_Z[iZ].Z = GetCalZOn_iZ_F(iZ, freq);
			pIndZwith_uint_Z[iZ].iZ = iZ;
		}
	}
	qsort((void*)(pIndZwith_uint_Z), nZ_cal, sizeof(struct IndZwith_uint_Z_str), compare_structs_on_uint_Z_and_iZ);
}

/*********************************************************************//**
* @brief        	������� ���������� ��� ������� ��������� ���� IndZwith_uint_Z_str �� �������� ������ ���������.
	@param[arg1]		��������� �� ������ ��������� ���� IndZwith_uint_Z_str.
	@param[arg2]		��������� �� ������ ��������� ���� IndZwith_uint_Z_str.
* @return       	���������� ������� ���� arg1->Z > arg2->, ����� ������� � ��������� ������ � 0 � ������ �� ���������
**********************************************************************/
int compare_structs_on_uint_Z_and_iZ(const void *arg1, const void *arg2)
{
	int ret;
	if (((struct IndZwith_uint_Z_str*)(arg1))->Z > ((struct IndZwith_uint_Z_str*)(arg2))->Z)
	{
		ret = 1;
	}
	else if (((struct IndZwith_uint_Z_str*)(arg1))->Z < ((struct IndZwith_uint_Z_str*)(arg2))->Z)
	{
		ret = -1;
	}
	else
	{
		ret = 0;
	}
	return ret;
}

/*********************************************************************//**
* @brief        	������� ���������� ��� ������� ��������� ���� IndZwith_float_Z_str �� �������� ������ ���������.
	@param[arg1]		��������� �� ������ ��������� ���� IndZwith_float_Z_str.
	@param[arg2]		��������� �� ������ ��������� ���� IndZwith_float_Z_str.
* @return       	���������� ������� ���� arg1->Z > arg2->, ����� ������� � ��������� ������ � 0 � ������ �� ���������
**********************************************************************/
int compare_structs_on_float_Z_and_iZ(const void *arg1, const void *arg2)
{
	int ret;
	if (((struct IndZwith_float_Z_str*)(arg1))->Z > ((struct IndZwith_float_Z_str*)(arg2))->Z)
	{
		ret = 1;
	}
	else if (((struct IndZwith_float_Z_str*)(arg1))->Z < ((struct IndZwith_float_Z_str*)(arg2))->Z)
	{
		ret = -1;
	}
	else
	{
		ret = 0;
	}
	return ret;
}

///*********************************************************************//**
//* @brief        	������� ���������� ������������� �������� ������ ��������� �� ������� cal_freq_list[freq_index]
//									��� ������������� �������� � �������� iZ
//	@param[freq_index]		������ �������
//	@param[iZ]		������ ������������� ��������
//* @return       	������������� �������� ������ ��������� (uint32_t)
//**********************************************************************/
//float GetZOn_iF_for_iZ (uint8_t freq_index, uint8_t iZ)
//{
//	float ret;
//	if (freq_index < CalData[iZ].nFmin)
//	{
//		ret = CalData[iZ].gZmax;
//	}
//	else if (freq_index >= CalData[iZ].nFmax)
//	{
//		ret = CalData[iZ].gZmin;
//	}
//	else
//	{
//		ret = CalData[iZ].Zarray[freq_index - CalData[iZ].nFmin].Zmin;
//	}
//	return ret;
//}

/*********************************************************************//**
* @brief        	������� ���������� �������� ������ ��������� ��������, ���������� �������� Z ������ ���
									� �������������� ������������� ������ ��� �������� � �������� iZ.
									������������� ������ ����� ��� ������� cal_freq_list[freq_index]
									��� ��������, 
	@param[freq_index]		������ �������
	@param[iZ]		������ ������������� ��������
* @return       	������������� �������� ������ ��������� (uint32_t)
**********************************************************************/
float GetZOn_iF_for_Z (uint8_t freq_index, uint16_t Z, uint8_t iZ)
{
	float ret;

	
	if (freq_index < CalData[iZ].nFmin)
	{
		ret = Z*CalData[iZ].Zarray[0].k+CalData[iZ].Zarray[0].b;
	}
	else if (freq_index >= CalData[iZ].nFmax)
	{
		ret = Z*CalData[iZ].Zarray[CalData[iZ].nFmax - 1 - CalData[iZ].nFmin].k+CalData[iZ].Zarray[CalData[iZ].nFmax - 1 - CalData[iZ].nFmin].b;
	}
	else
	{
		ret = Z*CalData[iZ].Zarray[freq_index - CalData[iZ].nFmin].k+CalData[iZ].Zarray[freq_index - CalData[iZ].nFmin].b;
	}
				
	return ret;
}

/*********************************************************************//**
* @brief        	������� ���������� ������������� �������� ������ ��������� �� ������� freq
									��� ������������� �������� � �������� iZ
	@param[freq]		�������, ���
	@param[iZ]			������ ������������� ��������
* @return       	������������� �������� ������ ��������� (float)
**********************************************************************/
float GetCalZOn_iZ_F (uint8_t iZ, uint16_t freq)
{
	float ret, t1,t2;
	uint8_t freq_index=0;
	float f_coef;
	
	while (cal_freq_list[freq_index] < freq)
	{
		freq_index++;
	}
	if (freq_index != 0)
	{
		freq_index--;
	}
	
	f_coef = ((float)(freq - cal_freq_list[freq_index]))/(cal_freq_list[freq_index+1] - cal_freq_list[freq_index]);
	
	if (freq_index < (nF_cal-1))
	{
		t1 = GetCalZOn_iZ_iF(iZ, freq_index);
		t2 = GetCalZOn_iZ_iF(iZ, freq_index+1);
		ret = t1 + (t2 - t1) * f_coef;;
	}
	else ret = GetCalZOn_iZ_iF(iZ, freq_index);
	//��������, ������ �� ������� freq � ������������� ��������� ������ ��� �������� iZ
	if (freq_index < CalData[iZ].nFmin)
	{
		ret = 1000000;
	}
	else if (freq_index+1 > CalData[iZ].nFmax)
	{
		ret = 1000000;
	}
	
	return ret;
}



/*********************************************************************//**
* @brief        	������� ���������� ������������� �������� ���� ��������� �� ������� freq
									��� ������������� �������� � �������� iZ
	@param[freq]		�������, ���
	@param[i_F_min]	������ ������ ������� ������� �� ��������� ���, � ������� �������� ������� freq
	@param[iZ]			������ ������������� ��������
* @return       	������������� �������� ���� ��������� (float)
**********************************************************************/
float GetPHOn_F_for_iZ (uint16_t freq, uint8_t i_F_min, uint8_t iZ)
{
	
	float ret;
	uint8_t freq_index=0;
	
	while (cal_freq_list[freq_index] < freq)
	{
		freq_index++;
	}
	if (freq_index != 0)
	{
		freq_index--; //������ freq_index ����� ������� ������� ���������� ������� cal_freq_list, � �������
								//��������� ������� freq
	}
	
	if (freq_index < CalData[iZ].nFmin)
	{
		ret = CalData[iZ].PHarray[0].PHmin;
	}
	else if ((freq_index+1) > (CalData[iZ].nFmax-1))
	//���� ������� ������ ���������� �������, � ������� ��������� ������� freq ��������� ������ �������
	//������������� ������� ��� ������ ������������� ��������
	{
		ret = CalData[iZ].PHarray[ CalData[iZ].nFmax - CalData[iZ].nFmin - 1 ].PHmin;
	}
	else
	{
		ret = CalData[iZ].PHarray[ freq_index - CalData[iZ].nFmin ].PHmin + (CalData[iZ].PHarray[ freq_index + 1 - CalData[iZ].nFmin ].PHmin - CalData[iZ].PHarray[ freq_index - CalData[iZ].nFmin ].PHmin) * ((float)(freq - cal_freq_list[freq_index]))/(cal_freq_list[freq_index+1] - cal_freq_list[freq_index]);;
	}
	
	return ret;
}

/*********************************************************************//**
* @brief        	������� �������� ������ ��������� � ������������ ���������� �������� � �������������� �������������
									�������������
	@param[freq]		������� ������� DDS, ��� (uint16_t). ������ ���� ������ ������� �������, �� ������� ������������� ����������
* @return       	������������ �������� ������ � ���� ��������� (float) results[0] � results[0] ��������������.
**********************************************************************/
void Measure(float results[2], uint16_t freq)
{
	uint16_t mag, ph;
	uint8_t freq_index=0;		//������ ������������� �������, ������� ��������� � freq, �� ������ ���
	
	uint8_t	I_Zcal_min_1=0;		//������ ������������� ��������, ������� ����������� ������������� �� ������ ������� ���������
	uint8_t	I_Zcal_min_2=0;		//������ ������������� ��������, ������� ����������� ������������� �� ������� ������� ���������
	uint8_t	I_Zcal_max_1=0;		//������ ������������� ��������, ������� ������������ ������������� �� ������ ������� ���������
	uint8_t	I_Zcal_max_2=0;		//������ ������������� ��������, ������� ������������ ������������� �� ������� ������� ���������
	
	uint32_t temp_adc;

	float f_coef, z_coef_1, z_coef_2;
	
	float Zmin_on_cur_F, Zmax_on_cur_F;
	
	struct IndZwith_uint_Z_str * pIndZwith_uint_Z;
	
	float ZcalOnLowFreqLowIndex, ZcalOnLowFreqHighIndex, ZcalOnHighFreqLowIndex, ZcalOnHighFreqHighIndex;
	float ZmeasuredOnLowFreqLowIndex, ZmeasuredOnLowFreqHighIndex, ZmeasuredOnHighFreqLowIndex, ZmeasuredOnHighFreqHighIndex;
	
	float PHcalOnLowFreqLowIndex, PHcalOnLowFreqHighIndex, PHcalOnHighFreqLowIndex, PHcalOnHighFreqHighIndex;
	float PHmeasuredOnLowFreqLowIndex, PHmeasuredOnLowFreqHighIndex, PHmeasuredOnHighFreqLowIndex, PHmeasuredOnHighFreqHighIndex;
	
	pIndZwith_uint_Z = malloc(sizeof(struct IndZwith_uint_Z_str)*nZ_cal);
	
	temp_adc = ADC_RUN(os);
	ph = (uint16_t)((temp_adc & 0xffff0000) >> 16);
	mag = (uint16_t)(temp_adc & 0xffff);
	//mag = 40780;
	//ph = 8857;
	
	if (freq < cal_freq_list[nF_cal-1])
	//���� ������� ������������� ������� �� ����� ������������ ������������� �������
	{
		while (cal_freq_list[freq_index] <= freq)
		{
			freq_index++;
		}
		if (freq_index != 0)
		{
			freq_index--;
		}
		//������ freq_index - ������ ������ ������� ���������� �������, ������� �������� ������������� ������ freq.
		
		SortZIndOnCurF (pIndZwith_uint_Z, freq);
		//����������� ������ �������� ������� ������������� �������� � �� �������� �� ������ �������.
		//// ��������!!!!!!!
		
		f_coef = ((float)(freq - cal_freq_list[freq_index]))/(cal_freq_list[freq_index+1] - cal_freq_list[freq_index]);
		//f_coef ��� �����������, ������������ ������������� ������� freq ����� ��������� � ��� �������������� ���������
		
		I_Zcal_min_1 = Calc_iZ_for_Min_Z_OnCur_iF(freq_index);
		//I_Zcal_min_1 ��� ������ ������������� ��������, ���������� ����������� �������� �� ������� � �������� freq_index
		I_Zcal_max_1 = Calc_iZ_for_Max_Z_OnCur_iF(freq_index);
		//I_Zcal_min_1 ��� ������ ������������� ��������, ���������� ������������ �������� �� ������� � �������� freq_index

		I_Zcal_min_2 = Calc_iZ_for_Min_Z_OnCur_iF(freq_index+1);
		//I_Zcal_min_2 ��� ������ ������������� ��������, ���������� ����������� �������� �� ������� � �������� freq_index+1
		I_Zcal_max_2 = Calc_iZ_for_Max_Z_OnCur_iF(freq_index+1);
		//I_Zcal_min_2 ��� ������ ������������� ��������, ���������� ������������ �������� �� ������� � �������� freq_index+1
		
		Zmin_on_cur_F = GetCalZOn_iZ_iF(I_Zcal_min_1, freq_index) + ((int16_t)GetCalZOn_iZ_iF(I_Zcal_min_2, freq_index+1) - (int16_t)GetCalZOn_iZ_iF(I_Zcal_min_1, freq_index)) * (float)(freq - cal_freq_list[freq_index]) / (float)(cal_freq_list[freq_index+1] - cal_freq_list[freq_index]);

		//��������� ����� I_Zcal_min_1, freq_index � I_Zcal_min_2, freq_index+1 ������, ���������� �������� ���� �����
		// �� ������� freq (��������� � �������� ���)
		
		Zmax_on_cur_F = GetCalZOn_iZ_iF(I_Zcal_max_1, freq_index) + ((int16_t)GetCalZOn_iZ_iF(I_Zcal_max_2, freq_index+1) - (int16_t)GetCalZOn_iZ_iF(I_Zcal_max_1, freq_index)) * ((float)(freq - cal_freq_list[freq_index])) / (float)(cal_freq_list[freq_index+1] - cal_freq_list[freq_index]);
		//��������� ����� I_Zcal_max_1, freq_index � I_Zcal_max_2, freq_index+1 ������, ���������� �������� ���� �����
		// �� ������� freq (��������� � �������� ���)
		
		if (mag >= Zmax_on_cur_F)
		{
			results[0] = GetZForF_iZ_Zmeasured(freq, I_Zcal_max_1, mag) * (1 - f_coef);
			results[0] += GetZForF_iZ_Zmeasured(freq, I_Zcal_max_2, mag) * f_coef;
			if (debug_mode==1)
			{
				printf("\nmag >= Zmax_on_cur_F, I_Zcal_max_1 = %u, I_Zcal_max_2 = %u", I_Zcal_max_1,I_Zcal_max_2);
			}
		}
		else if (mag <= Zmin_on_cur_F)
		{
			results[0] = GetZForF_iZ_Zmeasured(freq, I_Zcal_min_1, mag) * (1 - f_coef);
			results[0] += GetZForF_iZ_Zmeasured(freq, I_Zcal_min_2, mag) * f_coef;
			if (debug_mode==1)
			{
				printf("\nmag <= Zmin_on_cur_F, I_Zcal_min_1 = %u, I_Zcal_min_2 = %u", I_Zcal_min_1, I_Zcal_min_2);
			}
		}
		else	//���� ������� � �������� ������������� ������ � ���������� ����� ������������� ��������, �������
					//�� ������� ������� ����� ������� ��������, ��� mag, � ����� ������������� ��������, �������
					//�� ������� ������� ����� ������� ��������, ��� mag.
					//��-���� ����� ���� �������� ��������� ���������� ������
		{
			//����� ������� ������, �� ������� ����� ��������������

			GetCorrectIndexes(pCorrectIndexes, freq, mag, ph);
			
			if (debug_mode==1)
			{
				printf("\npCorrectIndexes = %u %u", pCorrectIndexes[0], pCorrectIndexes[1]);
			}
			//pCorrectIndexes[0] - ������ ������������� ������, ���������� ��������� �
			//����������� ���������
			//pCorrectIndexes[1] - ������ ������������� ������, ���������� ������ �� �������� �
			//����������� ���������
			
			//!!!!!!!!!!!!!��� ��� ���� ������������� ������� �������� ��������, ��������������� �� ������� ������ - ������� ����,
			//������� ����, ������ ��������� ���� � ������ ��������� ����

			// ������� ���������� � ������� ���������:
			
			ZcalOnLowFreqLowIndex = GetCalZOn_iZ_iF(pCorrectIndexes[0], freq_index);
			ZcalOnLowFreqHighIndex = GetCalZOn_iZ_iF(pCorrectIndexes[1], freq_index);
			ZcalOnHighFreqLowIndex = GetCalZOn_iZ_iF(pCorrectIndexes[0], freq_index+1);
			ZcalOnHighFreqHighIndex = GetCalZOn_iZ_iF(pCorrectIndexes[1], freq_index+1);
			
			// ���� ���������� �������� ��������� ����� ������� �������������� ��������� � ��������� 
			// pCorrectIndexes[0] � pCorrectIndexes[1]
			if ( ((mag>ZcalOnLowFreqLowIndex) && (mag<ZcalOnLowFreqHighIndex)) || ((mag<ZcalOnLowFreqLowIndex) && (mag>ZcalOnLowFreqHighIndex)) )
			{
				if (ZcalOnLowFreqHighIndex>ZcalOnLowFreqLowIndex)
				{
					z_coef_1 = (float)((int16_t)mag - (int16_t)ZcalOnLowFreqLowIndex) / (float)((int16_t)ZcalOnLowFreqHighIndex - (int16_t)ZcalOnLowFreqLowIndex);
				}
				else
				{
					z_coef_1 = (float)((int16_t)mag - (int16_t)ZcalOnLowFreqHighIndex) / (float)((int16_t)ZcalOnLowFreqLowIndex - (int16_t)ZcalOnLowFreqHighIndex);
				}
				if (debug_mode==1)
				{
					printf("\nz_coef_1 = %f", z_coef_1);
				}
			}
			
			// ���� mag �� �������� ������ pCorrectIndexes[1]
			else if (abs(mag-ZcalOnLowFreqLowIndex) > abs(mag-ZcalOnLowFreqHighIndex))
			{
				if (debug_mode==1)
				{
					printf("\nz_coef_1 = 1");
				}
				z_coef_1 = 1;
			}
			
			// ����� (���� mag �� �������� ������ pCorrectIndexes[0])
			else
			{
				if (debug_mode==1)
				{
					printf("\nz_coef_1 = 0");
				}
				z_coef_1 = 0;
			}
			// z_coef_1 - �����������, ������������ ������������ ������ mag ����������� ��������� ����� ���������� ������� (����������
			// ���� � ������� GetCorrectIndexes. ������������ ������������ �� ������� freq_index - ������ ������� ���������, �
			// ������� �������� ������������� ������� freq.
			
			if ( ((mag>ZcalOnHighFreqLowIndex) && (mag<ZcalOnHighFreqHighIndex)) || ((mag<ZcalOnHighFreqLowIndex) && (mag>ZcalOnHighFreqHighIndex)) )
			{
				if (ZcalOnHighFreqHighIndex>ZcalOnHighFreqLowIndex)
				{
					z_coef_2 = (float)((int16_t)mag - (int16_t)ZcalOnHighFreqLowIndex) / (float)((int16_t)ZcalOnHighFreqHighIndex - (int16_t)ZcalOnHighFreqLowIndex);
				}
				else
				{
					z_coef_2 = (float)((int16_t)mag - (int16_t)ZcalOnHighFreqHighIndex) / (float)((int16_t)ZcalOnHighFreqLowIndex - (int16_t)ZcalOnHighFreqHighIndex);
				}
				if (debug_mode==1)
				{
					printf("\nz_coef_2 = %f", z_coef_2);
				}
			}
			else if (abs(mag-ZcalOnHighFreqLowIndex) > abs(mag-ZcalOnHighFreqHighIndex))
			{
				if (debug_mode==1)
				{
					printf("\nz_coef_2 = 1");
				}
				z_coef_2 = 1;
			}
			else
			{
				if (debug_mode==1)
				{
					printf("\nz_coef_2 = 0");
				}
				z_coef_2 = 0;
			}
			// z_coef_2 - �����������, ������������ ������������ ������ mag ����������� ��������� ����� ���������� ������� (����������
			// ���� � ������� GetCorrectIndexes. ������������ ������������ �� ������� freq_index+1 - ������� ������� ���������, �
			// ������� �������� ������������� ������� freq.
			
			ZmeasuredOnLowFreqLowIndex = GetZOn_iF_for_Z(freq_index, mag, pCorrectIndexes[0]);
			ZmeasuredOnLowFreqHighIndex = GetZOn_iF_for_Z(freq_index, mag, pCorrectIndexes[1]);
			ZmeasuredOnHighFreqLowIndex = GetZOn_iF_for_Z(freq_index+1, mag, pCorrectIndexes[0]);
			ZmeasuredOnHighFreqHighIndex = GetZOn_iF_for_Z(freq_index+1, mag, pCorrectIndexes[1]);
			
			if (debug_mode==1)
			{
				printf("\nZ_freq_index_pCorrectIndexes[0] = %f", ZmeasuredOnLowFreqLowIndex);
				printf("\nZ_freq_index_pCorrectIndexes[1] = %f", ZmeasuredOnLowFreqHighIndex);
				printf("\nZ_freq_index+1_pCorrectIndexes[0] = %f", ZmeasuredOnHighFreqLowIndex);
				printf("\nZ_freq_index+1_pCorrectIndexes[1] = %f", ZmeasuredOnHighFreqHighIndex);
			}
			
			results[0] = (1-f_coef) * ((1-z_coef_1)*GetZOn_iF_for_Z(freq_index, mag, pCorrectIndexes[0]) + z_coef_1*GetZOn_iF_for_Z(freq_index, mag, pCorrectIndexes[1])) + f_coef * ((1-z_coef_2)*GetZOn_iF_for_Z(freq_index+1, mag, pCorrectIndexes[0]) + z_coef_2 * GetZOn_iF_for_Z(freq_index+1, mag, pCorrectIndexes[1]));
			
			// ������ ���������� ���� ���������:
			
			PHcalOnLowFreqLowIndex = GetCalZOn_iZ_iF(pCorrectIndexes[0], freq_index);
			PHcalOnLowFreqHighIndex = GetCalZOn_iZ_iF(pCorrectIndexes[1], freq_index);
			PHcalOnHighFreqLowIndex = GetCalZOn_iZ_iF(pCorrectIndexes[0], freq_index+1);
			PHcalOnHighFreqHighIndex = GetCalZOn_iZ_iF(pCorrectIndexes[1], freq_index+1);
			
			// ���� ���� ����������� ��������� ��������� ����� ������� �������������� ��������� � ��������� 
			// pCorrectIndexes[0] � pCorrectIndexes[1]
			if ( ((ph>PHcalOnLowFreqLowIndex) && (mag<PHcalOnLowFreqHighIndex)) || ((mag<PHcalOnLowFreqLowIndex) && (mag>PHcalOnLowFreqHighIndex)) )
			{
				if (PHcalOnLowFreqHighIndex>PHcalOnLowFreqLowIndex)
				{
					z_coef_1 = (float)((int16_t)mag - (int16_t)PHcalOnLowFreqLowIndex) / (float)((int16_t)PHcalOnLowFreqHighIndex - (int16_t)PHcalOnLowFreqLowIndex);
				}
				else
				{
					z_coef_1 = (float)((int16_t)mag - (int16_t)PHcalOnLowFreqHighIndex) / (float)((int16_t)PHcalOnLowFreqLowIndex - (int16_t)PHcalOnLowFreqHighIndex);
				}
				if (debug_mode==1)
				{
					printf("\nz_coef_1 = %f", z_coef_1);
				}
			}
			
			// ���� mag �� �������� ������ pCorrectIndexes[1]
			else if (abs(mag-PHcalOnLowFreqLowIndex) > abs(mag-PHcalOnLowFreqHighIndex))
			{
				if (debug_mode==1)
				{
					printf("\nz_coef_1 = 1");
				}
				z_coef_1 = 1;
			}
			
			// ����� (���� mag �� �������� ������ pCorrectIndexes[0])
			else
			{
				if (debug_mode==1)
				{
					printf("\nz_coef_1 = 0");
				}
				z_coef_1 = 0;
			}
			// z_coef_1 - �����������, ������������ ������������ ������ mag ����������� ��������� ����� ���������� ������� (����������
			// ���� � ������� GetCorrectIndexes. ������������ ������������ �� ������� freq_index - ������ ������� ���������, �
			// ������� �������� ������������� ������� freq.
			
			if ( ((mag>PHcalOnHighFreqLowIndex) && (mag<PHcalOnHighFreqHighIndex)) || ((mag<PHcalOnHighFreqLowIndex) && (mag>PHcalOnHighFreqHighIndex)) )
			{
				if (PHcalOnHighFreqHighIndex>PHcalOnHighFreqLowIndex)
				{
					z_coef_2 = (float)((int16_t)mag - (int16_t)PHcalOnHighFreqLowIndex) / (float)((int16_t)PHcalOnHighFreqHighIndex - (int16_t)PHcalOnHighFreqLowIndex);
				}
				else
				{
					z_coef_2 = (float)((int16_t)mag - (int16_t)PHcalOnHighFreqHighIndex) / (float)((int16_t)PHcalOnHighFreqLowIndex - (int16_t)PHcalOnHighFreqHighIndex);
				}
				if (debug_mode==1)
				{
					printf("\nz_coef_2 = %f", z_coef_2);
				}
			}
			else if (abs(mag-PHcalOnHighFreqLowIndex) > abs(mag-PHcalOnHighFreqHighIndex))
			{
				if (debug_mode==1)
				{
					printf("\nz_coef_2 = 1");
				}
				z_coef_2 = 1;
			}
			else
			{
				if (debug_mode==1)
				{
					printf("\nz_coef_2 = 0");
				}
				z_coef_2 = 0;
			}
			// z_coef_2 - �����������, ������������ ������������ ������ mag ����������� ��������� ����� ���������� ������� (����������
			// ���� � ������� GetCorrectIndexes. ������������ ������������ �� ������� freq_index+1 - ������� ������� ���������, �
			// ������� �������� ������������� ������� freq.
			
			PHmeasuredOnLowFreqLowIndex = GetZOn_iF_for_Z(freq_index, mag, pCorrectIndexes[0]);
			PHmeasuredOnLowFreqHighIndex = GetZOn_iF_for_Z(freq_index, mag, pCorrectIndexes[1]);
			PHmeasuredOnHighFreqLowIndex = GetZOn_iF_for_Z(freq_index+1, mag, pCorrectIndexes[0]);
			PHmeasuredOnHighFreqHighIndex = GetZOn_iF_for_Z(freq_index+1, mag, pCorrectIndexes[1]);
			
			if (debug_mode==1)
			{
				printf("\nZ_freq_index_pCorrectIndexes[0] = %f", PHmeasuredOnLowFreqLowIndex);
				printf("\nZ_freq_index_pCorrectIndexes[1] = %f", PHmeasuredOnLowFreqHighIndex);
				printf("\nZ_freq_index+1_pCorrectIndexes[0] = %f", PHmeasuredOnHighFreqLowIndex);
				printf("\nZ_freq_index+1_pCorrectIndexes[1] = %f", PHmeasuredOnHighFreqHighIndex);
			}
			
			results[1] = (1-f_coef) * ((1-z_coef_1)*GetZOn_iF_for_Z(freq_index, mag, pCorrectIndexes[0]) + z_coef_1*GetZOn_iF_for_Z(freq_index, mag, pCorrectIndexes[1])) + f_coef * ((1-z_coef_2)*GetZOn_iF_for_Z(freq_index+1, mag, pCorrectIndexes[0]) + z_coef_2 * GetZOn_iF_for_Z(freq_index+1, mag, pCorrectIndexes[1]));
		}
	}
	else
	//���� ������� ������������� ������� ����� ������������ ������������� �������
	{
		freq_index = nF_cal - 1;
		SortZIndOnCurF (pIndZwith_uint_Z, freq);
		//����������� ������ �������� ������� ������������� �������� � �� �������� �� ������ �������.
		
		I_Zcal_min_1 = Calc_iZ_for_Min_Z_OnCur_iF(freq_index);
		//I_Zcal_min_1 ��� ������ ������������� ��������, ���������� ����������� �������� �� ������� � �������� freq_index
		I_Zcal_max_1 = Calc_iZ_for_Max_Z_OnCur_iF(freq_index);
		//I_Zcal_min_1 ��� ������ ������������� ��������, ���������� ������������ �������� �� ������� � �������� freq_index
		
		Zmin_on_cur_F = GetCalZOn_iZ_iF(I_Zcal_min_1, freq_index);
		//Zmin_on_cur_F ��� ����������� �������� ���, ���������� ��� ���������� �� ������������ �������.
		
		Zmax_on_cur_F = GetCalZOn_iZ_iF(I_Zcal_max_1, freq_index);
		//Zmax_on_cur_F ��� ������������ �������� ���, ���������� ��� ���������� �� ������������ �������.
		
		if (mag >= Zmax_on_cur_F)
		{
			results[0] = GetZForF_iZ_Zmeasured(freq, I_Zcal_max_1, mag);
		}
		else if (mag <= Zmin_on_cur_F)
		{
			results[0] = GetZForF_iZ_Zmeasured(freq, I_Zcal_min_1, mag);
		}
		else	//���� ���������� ����� ������������� ��������, ������� �� ������� ������� ����� ������� ��������,
					// ��� mag, � ����� ������������� ��������, ������� �� ������� ������� ����� ������� ��������, ��� mag.
					//��-���� ����� ���� �������� ��������� ���������� ������ �� ������������ ������������� �������
		{
			//����� ������� ������, �� ������� ����� ��������������
			GetCorrectIndexes(pCorrectIndexes, freq, mag, ph);
			//!!!!!!!!!!!!!��� ��� ���� �������������. ������� �������� ��������, ��������������� �� ���� ������:
			//������ ��������� ���� � ������ ��������� ����
			
			z_coef_1 = (mag - GetCalZOn_iZ_iF(pCorrectIndexes[0], freq_index)) / (GetCalZOn_iZ_iF(pCorrectIndexes[1], freq_index) - GetCalZOn_iZ_iF(pCorrectIndexes[0], freq_index));
			// z_coef_1 - �����������, ������������ ������������ ������ mag ����������� ��������� ����� ���������� ������� (����������
			// ���� � ������� GetCorrectIndexes. ������������ ������������ �� ������� freq_index - ������ ������� ���������, �
			// ������� �������� ������������� ������� freq.
			
			results[0] = (z_coef_1*(mag*CalData[pCorrectIndexes[0]].Zarray[freq_index].k+CalData[pCorrectIndexes[0]].Zarray[freq_index].b) + (1-z_coef_1)*(mag*CalData[pCorrectIndexes[1]].Zarray[freq_index].k+CalData[pCorrectIndexes[1]].Zarray[freq_index].b));
		}
	}
	free(pIndZwith_uint_Z);
}

void GetCorrectIndexes(uint8_t* pCorrectIndexes, uint16_t freq, float mag, float ph)
{
	//����� ������� ��� ������������� ������ ���, ����� ������ ���������� ���� ������������� ������ ���� �� ������
	//������� �� ����������� ������ ��������� � ��� ������� ����� ������� ���������� � ��������� 
	//������ ����������� ��������� � ��� ���� �� ������������� ��� ������ ������ ���� �����������
	uint8_t i;
	
	struct IndZwith_float_Z_str * IndZwith_float_Z;
	
	IndZwith_float_Z = malloc(sizeof(struct IndZwith_float_Z_str)*nZ_cal);

	for (i = 0; i < nZ_cal; i++)
	{
		IndZwith_float_Z[i].iZ = i;
		IndZwith_float_Z[i].Z = GetSumOtkl(mag, ph, freq, i);
	}
	
	//��������� ��������� ������ ��������
	qsort((void*)IndZwith_float_Z, nZ_cal, sizeof(struct IndZwith_float_Z_str), compare_structs_on_float_Z_and_iZ);
	//!!!!!!!!!!!!!!!!�������?????????????????
	pCorrectIndexes[0] = IndZwith_float_Z[0].iZ;//����� ������ ������ ����� pIndZwithOtkl[0].iZ
	if ((IndZwith_float_Z[1].Z / IndZwith_float_Z[0].Z) > 3)
	{
		pCorrectIndexes[1] = IndZwith_float_Z[0].iZ;
	}
	else
	{
		pCorrectIndexes[1] = IndZwith_float_Z[1].iZ;
	}
	free(IndZwith_float_Z);
}


/*********************************************************************//**
* @brief        	������� ������������ ����� ������� ���������� � ���������/100 ������ ����������� ��������� � ���
									���� �� ������������� ������������� ������ � �������� n_cal
	@param[mag]			���������� ������ ���������
	@param[ph]			���������� ���� ���������
	@param[freq]		������� ������� DDS, ��� (uint16_t). ������ ���� ������ ������� �������, �� ������� ������������� ����������
	@param[n_cal]		������ ������������� ��������
* @return       	����� ������� ���������� (float).
**********************************************************************/
float GetSumOtkl(float mag, float ph, uint16_t freq, uint8_t n_cal)
{
	float ret;
	float cal;
	uint8_t freq_index = 0;
	while (cal_freq_list[freq_index] < freq)
	{
		freq_index++;
	}
	if (freq_index != 0)
	{
		freq_index--;
	}
	cal = GetCalZOn_iZ_F (n_cal, freq);
	ret = fabs((cal-mag)/((float)mag));
	cal = GetPHOn_F_for_iZ (freq, freq_index, n_cal);
	cal = fabs((cal-ph)/((float)ph));
	ret = ret + cal/5; // ��������� ���������������� ����� ������� ���������� �� ����
	return ret;
}

uint32_t GetCalZOn_iZ_iF (uint8_t iZ, uint8_t iF)
{
	if (iF <= CalData[iZ].nFmin)
	{
		return CalData[iZ].gZmax;
	}
	else if (iF >= CalData[iZ].nFmax)
	{
		return CalData[iZ].gZmin;
	}
	else
	{
		return CalData[iZ].Zarray[iF - CalData[iZ].nFmin - 1].Zmin;
	}
}


/*********************************************************************//**
* @brief        	�������� �� ��������� ���������� �����������
	@param[t]				�������� � ��
* @return       	none
**********************************************************************/
void wait(uint32_t t)
{
	t=t*9230;		//���������� ��� ������ ��������. ������� �� �������� ������� 
	while (t>0)
	{
		t--;
	}
}

/*********************************************************************//**
* @brief        	������� ������������ �������� ������ ��������� �� ������ ������������� �������� iZ �� ������� freq
									��� ���������� ��������� Zmeasured
	@param[freq]		�������, ���
	@param[iZ]			������ ������������� ��������
	@param[Zmeasured]		���������� ��������, ��. ���
* @return       	�������� �������� ������ ����������� ���������
**********************************************************************/
float GetZForF_iZ_Zmeasured(uint16_t freq, uint8_t iZ, uint16_t Zmeasured)
{
	uint8_t freq_index = 0;
	float ret;
	
	while (cal_freq_list[freq_index] < freq)
	{
		freq_index++;
	}
	if (freq_index != 0)
	{
		freq_index--;
	}
	//������ freq_index - ������ ������ ������� ���������� �������, ������� �������� ������������� ������ freq.
	if (freq_index < CalData[iZ].nFmin)
	{
		ret = CalData[iZ].Zarray[0].k * Zmeasured + CalData[iZ].Zarray[0].b;
		return ret;
	}
	else if (freq_index >= CalData[iZ].nFmax)
	{
		ret = CalData[iZ].Zarray[CalData[iZ].nFmax - CalData[iZ].nFmin - 1].k * Zmeasured + CalData[iZ].Zarray[CalData[iZ].nFmax - CalData[iZ].nFmin - 1].b;
		return ret;
	}
	else
	{
		freq_index = freq_index - CalData[iZ].nFmin;
		//������ freq_index - ������ ������ ������� ���������� �������, ������������� ��������  ������� �������� ������������� ������ freq.
		ret = CalData[iZ].Zarray[freq_index].k * Zmeasured + CalData[iZ].Zarray[freq_index].b;
		return ret;
	}
}

/*********************************************************************//**
* @brief        	������� ������������ ����������� ����� ����������� ������ ��� ���������� ������������ ������
									��� �������� � ����������� C, Rt, Rb, Rs, Rp
	@param[]				none
* @return       	��������� ���� PrepareStructForZcal, ���������� ������� ����������� � ������������ ������,
									�� ������� ����� ����� ����������� � ����������� ��� ������� �� �������� ������������� ������
									����� ��� � ������
**********************************************************************/
struct PrepareStructForZcal_str GetPrepareStructForZcal (void)
{
	struct PrepareStructForZcal_str temp;
	uint8_t freq_counter;
	uint8_t first=1;
	float phase, a, b, Zreal, Zimag, magnitude, Xc;
	uint32_t freq;
	
	temp.RamSize = 0;
	temp.iFmax = nF_cal - 1;
	temp.iFmin = 0;	//����� ���� �� ���� ��������
	for (freq_counter = 0; freq_counter < nF_cal-1; freq_counter ++)
	{
		freq = cal_freq_list[freq_counter]*1000;
		a = 2*3.1415926*freq*C*0.000000000001*(Rp+Rs);
		b = 2*3.1415926*freq*C*0.000000000001*Rp*Rs;
		Zreal = Rt+Rb+(Rp+a*b)/(1+a*a);
		Zimag = (b-Rp*a)/(1+a*a);
		phase = atanf(Zimag/Zreal);
		magnitude = sqrt(powf(Zreal,2)+powf(Zimag,2));
		Xc = 1/(2*3.1415926*freq*C*0.000000000001);
		
		//����������� ����������� ���������� �������
		if ( ( (magnitude < 1200) && (Xc < (20 * Rp) ) ) || (C == 0) )	// if Zc<6*Rp or C=0
		{
			if (first==1)
			{
				first=0;
				temp.iFmin = freq_counter;
			}
			//����������� ������������ ���������� �������
			if ( ( (magnitude > 100) && (phase > (-50 * 3.1415926 / 180)) &&  (Xc > (((float)(Rp))/10)) ) || (C == 0))			// if Zc>Rp/6 or C=0
			{
				temp.RamSize += sizeof(Zarray_t);
			}
			else
			{
				temp.iFmax = freq_counter;
				break;
			}
		}
	}
	//temp.RamSize += sizeof(Zarray_t); ������-�� �������
	temp.iFmax = (C == 0) ? nF_cal-1 : temp.iFmax;
	return temp;
}


// END OF FILE

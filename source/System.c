#include "System.h"
#include "Flash_cal_adresses.h"

mag_ph_calc_calibr_struct_t MagPhcT_st;

// Calibration frequency list in kHz
uint32_t cal_freq_list[nF_cal] = { 10, 15, 20, 30, 40, 60, 80, 100, 120, 140, 160, 180, 200, 250, 300, 350, 400, 450, 500, 700, 1000,\
																		1400, 1900, 2500, 3200, 4000, 5000 };
																	 
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
uint8_t debug_mode = 0, raw_data_mode = 0, FirstCalStart = 1;
uint16_t nZ_cal = 0;
uint_fast8_t sdvig;
uint8_t freq_index=0;		//������ ������������� �������, ������� ��������� � freq, �� ������ ���
float f_coef;
uint32_t freq;

struct CalData_struct* CalData;
struct CalData_struct* CalDataCalibr;
																		
uint16_t pCorrectIndexes[2] = {0, 0};

uint8_t CorrectIndexesOverride;
uint16_t FirstOverrideIndex;
uint16_t SecondOverrideIndex;

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
		uint32_t temp2 = 0, temp3 = 0, adc_abs_counter=0, adc_ph_counter=0;
		
		//��������� ������������ ������ PCM ������� - �� ������-�� �������
		for (sdvig=5;sdvig<9;sdvig++)
		{
			for (j=0;j<32;j++)
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
								if (raw_data_mode ==1)
								{
									printf("\n%8u,",i2s_fifo_buf[i]);
								}
								if (adc_abs_counter < temp2)
								{
									SumAbs += i2s_fifo_buf[i];
									adc_abs_counter++;
								}
							}
							else
							{
								if (raw_data_mode ==1)
								{
									printf("%8u;",i2s_fifo_buf[i]);
								}
								if (adc_ph_counter < temp2)
								{
									SumPh += i2s_fifo_buf[i];
									adc_ph_counter++;
								}
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
	uint_fast8_t freq_counter;
	
	uint16_t mag;
	uint16_t ph;
	volatile uint32_t temp_adc;
	
	uint8_t temp2;
	uint16_t temp;
	
//declaration of external struct
	extern mag_ph_calc_calibr_struct_t MagPhcT_st;
	//declaration of external function, which receibe pointer to external struct
	extern void CalibrateMagPhaseCalcTheoretic(mag_ph_calc_calibr_struct_t *st);
	
	extern uint32_t cal_freq_list[];
	extern int8_t command;
	
	extern struct pt Calibration_pt;
	extern char cmdbuf [15];
	extern int uart_rcv_len_cnt;
	extern char c;
	extern uint8_t UART_pressed_enter;
	static uint16_t CalZCounter;
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
				}
				
				PrepareStructForZcal = GetPrepareStructForZcal();
								
				if (PrepareStructForZcal.RamSize == 0)
				{
					printf ("\nCapacitor value too big. Connect lower capacitance.\n>");
					wait(100);//wait 100 ms
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
					CalDataCalibr->nFmax = PrepareStructForZcal.iFmax + 1;
					CalDataCalibr->C = C;
					CalDataCalibr->Rp = Rp;
					CalDataCalibr->Rs = Rs;
					CalDataCalibr->Rt = Rt;
					CalDataCalibr->Rb = Rb;
					
					for (freq_counter = CalDataCalibr->nFmin; freq_counter < CalDataCalibr->nFmax; freq_counter ++)
					{
						freq = cal_freq_list[freq_counter] * 1000;
						temp_adc = DdsFreq;
						AD9833_SetFreq(freq);
						wait( (uint32_t)((float)10 * temp_adc/freq) );
						temp_adc = ADC_RUN(os);
						ph = (uint16_t)((temp_adc & 0xffff0000) >> 16);
						mag = (uint16_t)(temp_adc & 0xffff);
						printf("\n%4u %5u %5u", cal_freq_list[freq_counter], mag, ph);
						CalDataCalibr->Zarray[freq_counter - CalDataCalibr->nFmin].mag = mag;
						CalDataCalibr->Zarray[freq_counter - CalDataCalibr->nFmin].ph = ph;
					}
					CalZCounter++;	//Increment calibrating impedances counter
					
					printf("\nC  = %6u", CalDataCalibr->C);
					printf("\nR  = %5u", CalDataCalibr->Rp);
					printf("\nnFmin  = %4u", CalDataCalibr->nFmin);
					printf("\nnFmax  = %4u", CalDataCalibr->nFmax);
					for (temp2 = 0; temp2 < (CalDataCalibr->nFmax - CalDataCalibr->nFmin); temp2++)
					{
						printf("\nF, mag, ph = %5u, %6u, %6u", cal_freq_list[temp2+CalDataCalibr->nFmin], CalDataCalibr->Zarray[temp2].mag, CalDataCalibr->Zarray[temp2].ph);
					}
				
					if (CalZCounter == nZ_cal_max)
					{
						SaveCalData(CalDataCalibr, stop_saving);
						free(CalDataCalibr->Zarray);
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
					printf("\nC  = %6u", CalData[temp].C);
					printf("\nR  = %5u", CalData[temp].Rp);
					printf("\nnFmin  = %3u", CalData[temp].nFmin);
					printf("\nnFmax  = %3u", CalData[temp].nFmax);
					for (temp2 = 0; temp2 < (CalData[temp].nFmax - CalData[temp].nFmin); temp2++)
					{
						printf("\nF, mag, ph = %5u, %6u, %6u", cal_freq_list[temp2 + CalData[temp].nFmin], CalData[temp].Zarray[temp2].mag, CalData[temp].Zarray[temp2].ph);
					}
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
uint16_t Calc_iZ_for_Min_Z_OnCur_iF (uint8_t freq_index)
{
	uint16_t zcounter, z_index = 0;
	float min_Z, maybe_min_Z;

	min_Z = 1000000000;
	for (zcounter = 0; zcounter < nZ_cal; zcounter++)
	{
		if (freq_index > (CalData[zcounter].nFmax - 1))
			maybe_min_Z = CalData[zcounter].Zarray[(CalData[zcounter].nFmax - CalData[zcounter].nFmin - 1)].mag;
		else if (freq_index < CalData[zcounter].nFmin)
			maybe_min_Z = CalData[zcounter].Zarray[0].mag ;
		else
		{
			maybe_min_Z = CalData[zcounter].Zarray[(CalData[zcounter].nFmax - 1 - CalData[zcounter].nFmin - freq_index)].mag;
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
uint16_t Calc_iZ_for_Max_Z_OnCur_iF (uint8_t freq_index)
{
	uint16_t zcounter, z_index = 0;
	float max_Z, maybe_max_Z;
	
	max_Z = 0;
	for (zcounter = 0; zcounter < nZ_cal; zcounter++)
	{
		if (freq_index > (CalData[zcounter].nFmax - 1))
			maybe_max_Z = CalData[zcounter].Zarray[(CalData[zcounter].nFmax - CalData[zcounter].nFmin - 1)].mag;
		else if (freq_index < CalData[zcounter].nFmin)
			maybe_max_Z = CalData[zcounter].Zarray[0].mag;
		else
		{
			maybe_max_Z = CalData[zcounter].Zarray[(CalData[zcounter].nFmax - 1 - CalData[zcounter].nFmin - freq_index)].mag;
		}
		
		if (maybe_max_Z > max_Z)
		{
			z_index = zcounter;
			max_Z = maybe_max_Z;
		}
	}
	return z_index;
}

///*********************************************************************//**
//* @brief        	������� ��������� ������������� ������ �������� �������� ������ ��������� ������������� ��������
//									� �� ��������� �� ������ �������. ���������� ������� �������������� �� �������� �������� ���������
//									���������������� �������� �������������. ���� ��������� ������� �� ������������� ������� �� ������
//									������������� ������ (CalData_str.freq[]), ��������, �� �������� ����������� ����������
//									�������������� �� ���� �������� ������������� �� �������� �������� (��� ����������� ������).
//	@param[pIndZwithZ]		��������� �� ��������� ���� IndZwithZ_str. � �������� ������ ������� ���������� ������ ���
//												CalData_str.nZ �������� ������� ���� � ��������� ����������� �� ������ ���� ��������.
//	@param[freq]		������� � ���
//* @return       	none
//**********************************************************************/
//void SortZIndOnCurF (struct IndZwith_uint_Z_str * pIndZwith_uint_Z, uint16_t freq)
//{
//	uint8_t freq_counter=0, iZ;
//	
//	//����������, � ����� �� ��������, ������������ �� �������� ������� cal_freq_list, �������� freq
//	while (cal_freq_list[freq_counter] < freq)
//	{
//		freq_counter++;
//	}
//	
//	if (cal_freq_list[freq_counter] == freq	)	//���� ������������� ������� - ���� �� ������, �� ������� �������������
//																						//����������
//	{
//		for (iZ=0; iZ<nZ_cal; iZ++)
//		{
//			pIndZwith_uint_Z[iZ].Z = GetCalZ_on_iZ_iF (iZ, freq_counter);
//			pIndZwith_uint_Z[iZ].iZ = iZ;
//		}
//	}
//	else	//���� �� ������������� ������� �� ������������� ����������
//	{
//		freq_counter--;	//������ ����� ������� ��������� freq_counter, ���� �� ����� ��������� �� �������, ������� ����,
//										//��� freq
//		for (iZ=0; iZ<nZ_cal; iZ++)
//		{
//			pIndZwith_uint_Z[iZ].Z = GetCalZ_on_F_iZ(iZ, freq);
//			pIndZwith_uint_Z[iZ].iZ = iZ;
//		}
//	}
//	qsort((void*)(pIndZwith_uint_Z), nZ_cal, sizeof(struct IndZwith_uint_Z_str), compare_structs_on_uint_Z_and_iZ);
//}

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
//* @brief        	������� ���������� �������� ������ ��������� ��������, ���������� �������� Z ������ ���
//									� �������������� ������������� ������ ��� �������� � �������� iZ.
//									������������� ������ ����� ��� ������� cal_freq_list[freq_index]
//									��� ��������, 
//	@param[freq_index]		������ �������
//	@param[iZ]		������ ������������� ��������
//* @return       	������������� �������� ������ ��������� (float)
//**********************************************************************/
//float GetRealZ_on_iF_iZ_for_Z (uint8_t freq_index, uint16_t Z, uint8_t iZ)
//{
//	float ret;

//	if (freq_index < CalData[iZ].nFmin)
//	{
//		ret = Z*CalData[iZ].Zarray[0].k+CalData[iZ].Zarray[0].b;
//	}
//	else if (freq_index >= CalData[iZ].nFmax)
//	{
//		ret = Z*CalData[iZ].Zarray[CalData[iZ].nFmax - 1 - CalData[iZ].nFmin].k+CalData[iZ].Zarray[CalData[iZ].nFmax - 1 - CalData[iZ].nFmin].b;
//	}
//	else
//	{
//		ret = Z*CalData[iZ].Zarray[freq_index - CalData[iZ].nFmin].k+CalData[iZ].Zarray[freq_index - CalData[iZ].nFmin].b;
//	}
//				
//	return ret;
//}

float GetRealZ_on_F_iZ1_iZ2_for_Z(uint16_t freq, uint16_t iZ1, uint16_t iZ2, uint16_t Z)
{
	float Z_coef_low_freq, Z_coef_high_freq, Z1lf, Z1hf, Z2lf, Z2hf, magph[2];
	if (iZ1 != iZ2)
		{
			Z_coef_low_freq = ((float)((int32_t)Z - (int32_t)(CalData[iZ1].Zarray[freq_index - CalData[iZ1].nFmin].mag)))/((float)((int32_t)CalData[iZ2].Zarray[freq_index - CalData[iZ2].nFmin].mag - (int32_t)(CalData[iZ1].Zarray[freq_index - CalData[iZ1].nFmin].mag)));
			Z_coef_high_freq = ((float)((int32_t)Z - (int32_t)(CalData[iZ1].Zarray[freq_index - CalData[iZ1].nFmin + 1].mag)))/((float)((int32_t)CalData[iZ2].Zarray[freq_index - CalData[iZ2].nFmin + 1].mag - (int32_t)(CalData[iZ1].Zarray[freq_index - CalData[iZ1].nFmin + 1].mag)));
		}
		else
		{
			Z_coef_low_freq = 0;
			Z_coef_high_freq = 0;
		}
	GetRealCalMagPh(magph, cal_freq_list[freq_index], iZ1);
	Z1lf = 	magph[0];
	GetRealCalMagPh(magph, cal_freq_list[freq_index], iZ2);
	Z2lf = 	magph[0];
	
	GetRealCalMagPh(magph, cal_freq_list[freq_index+1], iZ1);
	Z1hf = 	magph[0];
	GetRealCalMagPh(magph, cal_freq_list[freq_index+1], iZ2);
	Z2hf = 	magph[0];
	
	return (Z_coef_high_freq * Z2hf + (1 - Z_coef_high_freq) * Z1hf) * f_coef + (Z_coef_low_freq * Z2lf + (1 - Z_coef_low_freq) * Z1lf) * (1 - f_coef);
}

float GetRealPH_on_F_iZ1_iZ2_for_PH(uint16_t freq, uint16_t iZ1, uint16_t iZ2, uint16_t PH)
{
	float PH_coef_low_freq, PH_coef_high_freq, PH1lf, PH1hf, PH2lf, PH2hf, magph[2];
	if (iZ1 != iZ1)
		{
			PH_coef_low_freq = ((float)((int32_t)PH - (int32_t)(CalData[iZ1].Zarray[freq_index - CalData[iZ1].nFmin].ph)))/((float)((int32_t)CalData[iZ2].Zarray[freq_index - CalData[iZ2].nFmin].ph - (int32_t)(CalData[iZ1].Zarray[freq_index - CalData[iZ1].nFmin].ph)));
			PH_coef_high_freq = ((float)((int32_t)PH - (int32_t)(CalData[iZ1].Zarray[freq_index - CalData[iZ1].nFmin + 1].ph)))/((float)((int32_t)CalData[iZ2].Zarray[freq_index - CalData[iZ2].nFmin + 1].ph - (int32_t)(CalData[iZ1].Zarray[freq_index - CalData[iZ1].nFmin + 1].ph)));
		}
		else
		{
			PH_coef_low_freq = 0;
			PH_coef_high_freq = 0;
		}
	GetRealCalMagPh(magph, cal_freq_list[freq_index], iZ1);
	PH1lf = 	magph[1];
	GetRealCalMagPh(magph, cal_freq_list[freq_index], iZ2);
	PH2lf = 	magph[1];
	
	GetRealCalMagPh(magph, cal_freq_list[freq_index+1], iZ1);
	PH1hf = 	magph[1];
	GetRealCalMagPh(magph, cal_freq_list[freq_index+1], iZ2);
	PH2hf = 	magph[1];
	
	return (PH_coef_high_freq * PH2hf + (1 - PH_coef_high_freq) * PH1hf) * f_coef + (PH_coef_low_freq * PH2lf + (1 - PH_coef_low_freq) * PH1lf) * (1 - f_coef);
}


///*********************************************************************//**
//* @brief        	������� ������������ �������� ������ ��������� �� ������ ������������� �������� iZ �� ������� freq
//									��� ���������� ��������� Z
//	@param[freq]		�������, ���
//	@param[iZ]			������ ������������� ��������
//	@param[Z]		���������� ��������, ��. ���
//* @return       	�������� �������� ������ ����������� ���������
//**********************************************************************/
//float GetRealZ_on_F_iZ_for_Z(uint16_t freq, uint8_t iZ, uint16_t Z)
//{
//	float ret;
//	
//		if (freq_index < CalData[iZ].nFmin)
//	{
//		ret = CalData[iZ].Zarray[0].k * Z + CalData[iZ].Zarray[0].b;
//		return ret;
//	}
//	else if (freq_index >= CalData[iZ].nFmax)
//	{
//		ret = CalData[iZ].Zarray[CalData[iZ].nFmax - CalData[iZ].nFmin - 1].k * Z + CalData[iZ].Zarray[CalData[iZ].nFmax - CalData[iZ].nFmin - 1].b;
//		return ret;
//	}
//	else
//	{
//		freq_index = freq_index - CalData[iZ].nFmin;
//		//������ freq_index - ������ ������ ������� ���������� �������, ������������� ��������  ������� �������� ������������� ������ freq.
//		ret = CalData[iZ].Zarray[freq_index].k * Z + CalData[iZ].Zarray[freq_index].b;
//		return ret;
//	}
//}

uint32_t GetCalZ_on_iZ_iF (uint16_t iZ, uint8_t freq_index)
{
	if (freq_index > (CalData[iZ].nFmax - 1))
	{
		return CalData[iZ].Zarray[(CalData[iZ].nFmax - 1 - CalData[iZ].nFmin)].mag;
	}
	else if (freq_index < CalData[iZ].nFmin)
	{
		return CalData[iZ].Zarray[0].mag;
	}
	else
	{
		return CalData[iZ].Zarray[freq_index - CalData[iZ].nFmin].mag;
	}
}

/*********************************************************************//**
* @brief        	������� ���������� ������������� �������� ������ ��������� �� ������� freq
									��� ������������� �������� � �������� iZ
	@param[freq]		�������, ���
	@param[iZ]			������ ������������� ��������
* @return       	������������� �������� ������ ��������� (float)
**********************************************************************/
float GetCalZ_on_F_iZ (uint16_t iZ, uint16_t freq)
{
	float ret, t1,t2;

	if (freq_index < (nF_cal-1))
	{
		t1 = GetCalZ_on_iZ_iF(iZ, freq_index);
		t2 = GetCalZ_on_iZ_iF(iZ, freq_index+1);
		ret = t1 + (t2 - t1) * f_coef;
	}
	else ret = GetCalZ_on_iZ_iF(iZ, freq_index);
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

uint32_t GetCalPH_on_iZ_iF (uint16_t iZ, uint8_t freq_index)
{
	if (freq_index > (CalData[iZ].nFmax - 1))
	{
		return CalData[iZ].Zarray[CalData[iZ].nFmax - 1 - CalData[iZ].nFmin].ph;
	}
	else if (freq_index < CalData[iZ].nFmin)
	{
		return CalData[iZ].Zarray[0].ph;
	}
	else
	{
		return CalData[iZ].Zarray[freq_index - CalData[iZ].nFmin].ph;
	}
}
/*********************************************************************//**
* @brief        	������� ���������� ������������� �������� ���� ��������� �� ������� freq
									��� ������������� �������� � �������� iZ
	@param[freq]		�������, ���
	@param[i_F_min]	������ ������ ������� ������� �� ��������� ���, � ������� �������� ������� freq
	@param[iZ]			������ ������������� ��������
* @return       	������������� �������� ���� ��������� (float)
**********************************************************************/
float GetCalPH_on_F_iZ (uint16_t iZ, uint16_t freq)
{
	float ret, t1,t2;

	if (freq_index < (nF_cal-1))
	{
		t1 = GetCalPH_on_iZ_iF(iZ, freq_index);
		t2 = GetCalPH_on_iZ_iF(iZ, freq_index+1);
		ret = t1 + (t2 - t1) * f_coef;;
	}
	else ret = GetCalPH_on_iZ_iF(iZ, freq_index);
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
///*********************************************************************//**
//* @brief        				������� ���������� �������� ���� ��������� ��������, ���������� ���� PH ������ ���
//												� �������������� ������������� ������ ��� �������� � �������� iZ.
//												������������� ������ ����� ��� ������� cal_freq_list[freq_index]
//	@param[freq_index]		������ �������
//	@param[iZ]						������ ������������� ��������
//* @return       				������������� �������� ���� ��������� (float)
//**********************************************************************/
//float GetRealPH_on_iF_iZ_for_PH (uint8_t freq_index, uint16_t PH, uint8_t iZ)
//{
//	float ret;

//	if (freq_index < CalData[iZ].nFmin)
//	{
//		ret = PH*CalData[iZ].PHarray[0].k+CalData[iZ].PHarray[0].b;
//	}
//	else if (freq_index >= CalData[iZ].nFmax)
//	{
//		ret = PH*CalData[iZ].PHarray[CalData[iZ].nFmax - 1 - CalData[iZ].nFmin].k+CalData[iZ].PHarray[CalData[iZ].nFmax - 1 - CalData[iZ].nFmin].b;
//	}
//	else
//	{
//		ret = PH*CalData[iZ].PHarray[freq_index - CalData[iZ].nFmin].k+CalData[iZ].PHarray[freq_index - CalData[iZ].nFmin].b;
//	}
//	return ret;
//}

///*********************************************************************//**
//* @brief        			������� ������������ �������� ���� ��������� �� ������ ������������� �������� iZ �� ������� freq
//											��� ���������� �������� ���� ��������� PH
//	@param[freq]				�������, ���
//	@param[iZ]					������ ������������� ��������
//	@param[PH]					���������� ��������, ��. ���
//* @return       			�������� �������� ������ ����������� ���������
//**********************************************************************/
//float GetRealPH_on_F_iZ_for_Z(uint16_t freq, uint8_t iZ, uint16_t PH)
//{
//	uint8_t freq_index = 0;
//	float ret;
//	
//	while (cal_freq_list[freq_index] < freq)
//	{
//		freq_index++;
//	}
//	if (freq_index != 0)
//	{
//		freq_index--;
//	}
//	//������ freq_index - ������ ������ ������� ���������� �������, ������� �������� ������������� ������ freq.
//	if (freq_index < CalData[iZ].nFmin)
//	{
//		ret = CalData[iZ].PHarray[0].k * PH + CalData[iZ].PHarray[0].b;
//		return ret;
//	}
//	else if (freq_index >= CalData[iZ].nFmax)
//	{
//		ret = CalData[iZ].PHarray[CalData[iZ].nFmax - CalData[iZ].nFmin - 1].k * PH + CalData[iZ].PHarray[CalData[iZ].nFmax - CalData[iZ].nFmin - 1].b;
//		return ret;
//	}
//	else
//	{
//		freq_index = freq_index - CalData[iZ].nFmin;
//		//������ freq_index - ������ ������ ������� ���������� �������, ������������� ��������  ������� �������� ������������� ������ freq.
//		ret = CalData[iZ].PHarray[freq_index].k * PH + CalData[iZ].PHarray[freq_index].b;
//		return ret;
//	}
//}

/*********************************************************************//**
* @brief        	������� �������� ������ ��������� � ������������ ���������� �������� � �������������� �������������
									�������������
	@param[freq]		������� ������� DDS, ��� (uint16_t). ������ ���� ������ ������� �������, �� ������� ������������� ����������
* @return       	������������ �������� ������ � ���� ��������� (float) results[0] � results[0] ��������������.
**********************************************************************/
void Measure(float results[2], uint16_t freq)
{
	uint16_t mag, ph;
	
	uint16_t	I_Zcal_min_1=0;		//������ ������������� ��������, ������� ����������� ������������� �� ������ ������� ���������
	uint16_t	I_Zcal_min_2=0;		//������ ������������� ��������, ������� ����������� ������������� �� ������� ������� ���������
	uint16_t	I_Zcal_max_1=0;		//������ ������������� ��������, ������� ������������ ������������� �� ������ ������� ���������
	uint16_t	I_Zcal_max_2=0;		//������ ������������� ��������, ������� ������������ ������������� �� ������� ������� ���������
	
	uint32_t temp_adc;

	float Zmin_on_cur_F, Zmax_on_cur_F;

	
	temp_adc = ADC_RUN(os);
	ph = (uint16_t)((temp_adc & 0xffff0000) >> 16);
	mag = (uint16_t)(temp_adc & 0xffff);
	
	if (freq < cal_freq_list[nF_cal-1])
	//���� ������� ������������� ������� �� ����� ������������ ������������� �������
	{
		freq_index = 0;
		while (cal_freq_list[freq_index] <= freq)
		{
			freq_index++;
		}
		if (freq_index != 0)
		{
			freq_index--;
		}
		//������ freq_index - ������ ������ ������� ���������� �������, ������� �������� ������������� ������ freq.
		
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
		
		Zmin_on_cur_F = GetCalZ_on_iZ_iF(I_Zcal_min_1, freq_index) + ((int16_t)GetCalZ_on_iZ_iF(I_Zcal_min_2, freq_index+1) - (int16_t)GetCalZ_on_iZ_iF(I_Zcal_min_1, freq_index)) * (float)(freq - cal_freq_list[freq_index]) / (float)(cal_freq_list[freq_index+1] - cal_freq_list[freq_index]);

		//��������� ����� I_Zcal_min_1, freq_index � I_Zcal_min_2, freq_index+1 ������, ���������� �������� ���� �����
		// �� ������� freq (��������� � �������� ���)
		
		Zmax_on_cur_F = GetCalZ_on_iZ_iF(I_Zcal_max_1, freq_index) + ((int16_t)GetCalZ_on_iZ_iF(I_Zcal_max_2, freq_index+1) - (int16_t)GetCalZ_on_iZ_iF(I_Zcal_max_1, freq_index)) * ((float)(freq - cal_freq_list[freq_index])) / (float)(cal_freq_list[freq_index+1] - cal_freq_list[freq_index]);
		//��������� ����� I_Zcal_max_1, freq_index � I_Zcal_max_2, freq_index+1 ������, ���������� �������� ���� �����
		// �� ������� freq (��������� � �������� ���)
		
		if (mag >= Zmax_on_cur_F)
		{
			results[0] = GetRealZ_on_F_iZ1_iZ2_for_Z(freq, I_Zcal_max_1, I_Zcal_max_2, mag);
			if (debug_mode==1)
			{
				printf("\nmag >= Zmax_on_cur_F, I_Zcal_max_1 = %u, I_Zcal_max_2 = %u", I_Zcal_max_1,I_Zcal_max_2);
			}
		}
		else if (mag <= Zmin_on_cur_F)
		{
			results[0] = GetRealZ_on_F_iZ1_iZ2_for_Z(freq, I_Zcal_min_1, I_Zcal_min_2, mag);
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
			if (CorrectIndexesOverride == 1)
			{
				pCorrectIndexes[0] = FirstOverrideIndex;
				pCorrectIndexes[1] = SecondOverrideIndex;
			}
			
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

			results[0] = GetRealZ_on_F_iZ1_iZ2_for_Z(freq, pCorrectIndexes[0], pCorrectIndexes[1], mag);
			results[1] = GetRealPH_on_F_iZ1_iZ2_for_PH(freq, pCorrectIndexes[0], pCorrectIndexes[1], ph);
		}
	}
}

void GetCorrectIndexes(uint16_t* pCorrectIndexes, uint16_t freq, float mag, float ph)
{
	//����� ������� ��� ������������� ������ ���, ����� ������ ���������� ���� ������������� ������ ���� �� ������
	//������� �� ����������� ������ ��������� � ��� ������� ����� ������� ���������� � ��������� 
	//������ ����������� ��������� � ��� ���� �� ������������� ��� ������ ������ ���� �����������
	uint8_t counter, success = 0;
	uint16_t i;
	uint16_t SecondDeriviate;
	
	struct IndZwith_float_Z_str * IndZwith_float_Z;
	
	IndZwith_float_Z = malloc(sizeof(struct IndZwith_float_Z_str)*nZ_cal);

	for (i = 0; i < nZ_cal; i++)
	{
		IndZwith_float_Z[i].iZ = i;
		IndZwith_float_Z[i].Z = GetSumOtkl(mag, ph, freq, i);
	}
	
	//��������� ��������� ������ ��������
	qsort((void*)IndZwith_float_Z, nZ_cal, sizeof(struct IndZwith_float_Z_str), compare_structs_on_float_Z_and_iZ);
	
	pCorrectIndexes[0] = IndZwith_float_Z[0].iZ;//����� ������ ������ ����� pIndZwithOtkl[0].iZ
	
	if (IndZwith_float_Z[0].Z * 10 < IndZwith_float_Z[1].Z)
	{
		pCorrectIndexes[1] = pCorrectIndexes[0];
	}
	else
	{
	if (GetCalZ_on_F_iZ(pCorrectIndexes[0], freq)< mag)
		{
			for (counter = 2;counter < 5; counter++)
			{
				if ( (GetCalZ_on_F_iZ(IndZwith_float_Z[counter].iZ, freq) > mag) && ( ( (GetCalPH_on_F_iZ(IndZwith_float_Z[counter].iZ, freq) > ph) && (GetCalPH_on_F_iZ(IndZwith_float_Z[0].iZ, freq) < ph) ) || ( (GetCalPH_on_F_iZ(IndZwith_float_Z[counter].iZ, freq) < ph) && (GetCalPH_on_F_iZ(IndZwith_float_Z[0].iZ, freq) > ph) ) ) )
					{
						pCorrectIndexes[1] = IndZwith_float_Z[counter].iZ;
						SecondDeriviate = IndZwith_float_Z[counter].Z;
						success = 1;
						if (debug_mode==1)
							{
								printf("\nCorrect index 2 count on Map Ph = %u", counter);
							}
						break;
					}
			}
			if (success != 1)
			{
				for (counter = 2;counter < 5; counter++)
				{
					if ( GetCalZ_on_F_iZ(IndZwith_float_Z[counter].iZ, freq) > mag )
						{
							pCorrectIndexes[1] = IndZwith_float_Z[counter].iZ;
							SecondDeriviate = IndZwith_float_Z[counter].Z;
							success = 1;
							if (debug_mode==1)
							{
								printf("\nCorrect index 2 count on Map = %u", counter);
							}
							break;
						}
				}
			}
			if (success != 1)
			{
				pCorrectIndexes[1] = pCorrectIndexes[0];
				SecondDeriviate = IndZwith_float_Z[0].Z;
			}
		}
		else
		{
			for (counter = 2;counter < 5; counter++)
			{
				if ( (GetCalZ_on_F_iZ(IndZwith_float_Z[counter].iZ, freq) < mag) && ( ( (GetCalPH_on_F_iZ(IndZwith_float_Z[counter].iZ, freq) > ph) && (GetCalPH_on_F_iZ(IndZwith_float_Z[0].iZ, freq) < ph) ) || ( (GetCalPH_on_F_iZ(IndZwith_float_Z[counter].iZ, freq) < ph) && (GetCalPH_on_F_iZ(IndZwith_float_Z[0].iZ, freq) > ph) ) ) )
					{
						pCorrectIndexes[1] = IndZwith_float_Z[counter].iZ;
						SecondDeriviate = IndZwith_float_Z[counter].Z;
						success = 1;
						if (debug_mode==1)
							{
								printf("\nCorrect index 2 count on Map Ph = %u", counter);
							}
						break;
					}
			}
			if (success != 1)
			{
				for (counter = 2;counter < 5; counter++)
				{
					if ( GetCalZ_on_F_iZ(IndZwith_float_Z[counter].iZ, freq) < mag )
						{
							pCorrectIndexes[1] = IndZwith_float_Z[counter].iZ;
							SecondDeriviate = IndZwith_float_Z[counter].Z;
							success = 1;
							if (debug_mode==1)
							{
								printf("\nCorrect index 2 count on Map = %u", counter);
							}
							break;
						}
				}
			}
			if (success != 1)
			{
				pCorrectIndexes[1] = pCorrectIndexes[0];
				SecondDeriviate = IndZwith_float_Z[0].Z;
			}
		}
	}
	
	if ( ( GetMagOtkl(mag, freq, pCorrectIndexes[1]) / GetMagOtkl(mag, freq, IndZwith_float_Z[1].iZ) > 5 ) || ( GetPhOtkl(ph, freq, pCorrectIndexes[1]) / GetPhOtkl(ph, freq, IndZwith_float_Z[1].iZ) > 5 ) )
	{
		pCorrectIndexes[1] = IndZwith_float_Z[0].iZ;
	}
	
	if (debug_mode==1)
		{
			printf("\npSortedCurvesIndexesDeviation:\n%6.6f %6.6f %6.6f %6.6f %6.6f %6.6f %6.6f", IndZwith_float_Z[0].Z, IndZwith_float_Z[1].Z, IndZwith_float_Z[2].Z, IndZwith_float_Z[3].Z, IndZwith_float_Z[4].Z, IndZwith_float_Z[5].Z, IndZwith_float_Z[6].Z);
			printf("\npSortedCurvesIndexes:\n%u %u %u %u %u %u %u", IndZwith_float_Z[0].iZ, IndZwith_float_Z[1].iZ, IndZwith_float_Z[2].iZ, IndZwith_float_Z[3].iZ, IndZwith_float_Z[4].iZ, IndZwith_float_Z[5].iZ, IndZwith_float_Z[6].iZ);
			printf("\npCorrectIndexesDeviation = %f %u", IndZwith_float_Z[0].Z, SecondDeriviate);
		}
	free(IndZwith_float_Z);
}
		
/*********************************************************************//**
* @brief        	������� ������������ ����� ������� ���������� � ���������/100 ������ ����������� ��������� � ���
									���� �� ������������� ������������� ������ � �������� iZ
	@param[mag]			���������� ������ ���������
	@param[ph]			���������� ���� ���������
	@param[freq]		������� ������� DDS, ��� (uint16_t). ������ ���� ������ ������� �������, �� ������� ������������� ����������
	@param[iZ]		������ ������������� ��������
* @return       	����� ������� ���������� (float).
**********************************************************************/
float GetSumOtkl(float mag, float ph, uint16_t freq, uint16_t iZ)
{
	float ret;
	float cal;

	cal = GetCalZ_on_F_iZ (iZ, freq);
	ret = fabs(cal-mag)/sqrtf(mag);
	cal = GetCalPH_on_F_iZ (iZ, freq);
	cal = fabs(cal-ph)/sqrtf(ph);
	ret = ret + cal/1; // ��������� ���������������� ����� ������� ���������� �� ����
	return ret;
}
float GetMagOtkl(float mag, uint16_t freq, uint16_t iZ)
{
	float ret;

	ret = GetCalZ_on_F_iZ (iZ, freq);
	ret = fabs(ret-mag);
	return ret;
}
float GetPhOtkl(float ph, uint16_t freq, uint16_t iZ)
{
	float ret;

	ret = GetCalPH_on_F_iZ (iZ, freq);
	ret = fabs(ret-ph);
	return ret;
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
	for (freq_counter = 0; freq_counter < nF_cal; freq_counter ++)
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
		if ( ( (magnitude < 1500) && (Xc < (200 * Rp) ) ) || (C == 0) )	// if Zc<200*Rp or C=0
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

void GetRealCalMagPh(float result[2], uint16_t freq, uint16_t iZ)
{
	float C, Rp, Rs, a, b, real, imag;
	
	C = CalData[iZ].C;
	Rp = CalData[iZ].Rp;
	Rs = CalData[iZ].Rs;
	Rt = CalData[iZ].Rt;
	Rb = CalData[iZ].Rb;
	a = 2 * 3.1415926 * freq * 1000 * C * 0.000000000001 * (Rp + Rs);
	b = 2 * 3.1415926 * freq * 1000 * C * 0.000000000001 * Rp * Rs;
	real = (Rp + a*b)/(1+a*a);	//��������. ����� ���������
	imag = (b - Rp * a) / (1 + a * a);													  //������ ����� ���������
	result[0] = sqrt(powf(real,2)+powf(imag,2));															//������ ���������
	result[1] = atanf(imag/real);																							//���� ���������
}
// END OF FILE

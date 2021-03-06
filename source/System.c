#include "System.h"
#include "Flash_cal_adresses.h"

mag_ph_calc_calibr_struct_t MagPhcT_st;

// Calibration frequency list in kHz
const uint32_t cal_freq_list[nF_cal] = { 10, 15, 20, 30, 40, 60, 80, 100, 120, 140, 160, 180, 200, 250, 300, 350, 400, 450, 500, 700, 1000,\
																		1400, 1900, 2500, 3200, 4000, 5000 };

const uint32_t cal_cap_list[nCap_cal] = { 0, 7, 18, 27, 39, 47, 68, 82, 100, 120, 150, 220, 268, 330, 390, 470, 560, 680, 820, 1020,\
																		1180, 1510, 1790, 2200, 2700, 3300, 3920, 4680, 5700, 6800, 8120, 9900, 12020, 15100, 18050, 22030 };

const uint32_t cal_par_list[nPar_cal] = { 221, 270, 328, 387, 430, 467, 510, 560, 619, 675, 748, 820, 912, 995, 1099, 1196, 1298 };

																		
uint32_t C=0;
uint32_t Rt = 10;
uint32_t Rb = 10;
uint32_t Rs = 0;
uint32_t Rp = 1106;
struct IndZwithZ_str * pIndZwithZ;
extern void LoadCalData(void);
volatile uint32_t DdsFreq;
uint8_t debug_mode = 0, raw_data_mode = 0, FirstCalStart = 1;
uint16_t nZ_cal = 0;
uint8_t freq_index=0;		//������ ������������� �������, ������� ��������� � freq, �� ������ ���
float f_coef;
uint32_t freq;
																		
uint16_t mag;
uint16_t ph;

struct CalData_struct* CalData;
struct CalData_struct* CalDataCalibr;
																		
uint16_t pCorrectIndexes[2] = {0, 0};
uint16_t pMagCI[2] = {0, 0};
uint16_t pPHCI[2] = {0, 0};

uint8_t CorrectIndexesOverride;
uint8_t CorrectIndexesBrute;
uint8_t EqualIndexes;
uint16_t FirstOverrideIndex;
uint16_t SecondOverrideIndex;

extern int8_t command;

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
	
	void ADC_RUN(uint32_t* result)
	{
		uint32_t MeanAbs, MeanPh;
		// Add code here
		
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

	void led_on(void)
	{
		GPIO_SetValue (0, (1<<0));
	}
	
	void led_off(void)
		{
			GPIO_ClearValue (0, (1<<0));
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
	
	uint32_t result[2];
	volatile uint32_t temp_freq;
	
	uint8_t temp2;
	uint16_t temp;
	
//declaration of external struct
	extern mag_ph_calc_calibr_struct_t MagPhcT_st;
	//declaration of external function, which receibe pointer to external struct
	extern void CalibrateMagPhaseCalcTheoretic(mag_ph_calc_calibr_struct_t *st);
	
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
						temp_freq = DdsFreq;
						AD9833_SetFreq(freq);
						wait( (uint32_t)((float)10 * temp_freq/freq) );
						ADC_RUN(result);
						ph = result[1];
						mag = result[0];
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
	if (iZ1 != iZ2)
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
	uint16_t	I_Zcal_min_1=0;		//������ ������������� ��������, ������� ����������� ������������� �� ������ ������� ���������
	uint16_t	I_Zcal_min_2=0;		//������ ������������� ��������, ������� ����������� ������������� �� ������� ������� ���������
	uint16_t	I_Zcal_max_1=0;		//������ ������������� ��������, ������� ������������ ������������� �� ������ ������� ���������
	uint16_t	I_Zcal_max_2=0;		//������ ������������� ��������, ������� ������������ ������������� �� ������� ������� ���������
	
	uint8_t MCount;

	float Zmin_on_cur_F, Zmax_on_cur_F;
	
	uint32_t result[2];

	ADC_RUN(result);
	
	ph = result[1];
	mag = result[0];
	
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
			if (CorrectIndexesBrute == 1)
			{
				MCount = 100;
			}
			else
			{
				MCount = 1;
			}
			while (MCount > 0)
			{
				MCount--;
				//����� ������� ������, �� ������� ����� ��������������
				GetCorrectIndexes(pMagCI, pPHCI, freq, mag, ph);
				if (CorrectIndexesOverride == 1)
				{
					pMagCI[0] = FirstOverrideIndex;
					pPHCI[0] = FirstOverrideIndex;
					pMagCI[1] = SecondOverrideIndex;
					pPHCI[1] = SecondOverrideIndex;
				}
				if (EqualIndexes == 1)
				{
					pPHCI[0] = pMagCI[0];
					pPHCI[1] = pMagCI[1];
				}
				if (debug_mode==1)
				{
					printf("\npMagCI = %u %u", pMagCI[0], pMagCI[1]);
					printf("\npPHCI = %u %u", pPHCI[0], pPHCI[1]);
				}
				results[0] = GetRealZ_on_F_iZ1_iZ2_for_Z(freq, pMagCI[0], pMagCI[1], mag);
				results[1] = GetRealPH_on_F_iZ1_iZ2_for_PH(freq, pPHCI[0], pPHCI[1], ph);
				
				if (CorrectIndexesBrute == 1)
					{
						if (MCount == 99)
						{
							printf("\npMagCI[0] pMagCI[1] pPHCI[0] pPHCI[1]  mag      ph");
						}
						printf("\n%-9u %-9u %-8u %-9u %-7.3f %-10.3f", pMagCI[0], pMagCI[1], pPHCI[0], pPHCI[1], results[0], results[1]*57.295779513);
					}
			}
		}
	}
}

void GetCorrectIndexes(uint16_t* pMagCI, uint16_t* pPHCI, uint16_t freq, float mag, float ph)
{
	uint16_t i;
	
	struct IndZwith_float_Z_str * CIarray;
	
	static uint8_t FirstCICounter = 0, SecondCICounter = 0;
	
	uint8_t LowCurveCapIndex;
	uint8_t LowCurveParIndex;
	
	CIarray = malloc(sizeof(struct IndZwith_float_Z_str)*nZ_cal);
	
	for (i = 0; i < nZ_cal; i++)
	{
		CIarray[i].iZ = i;
		CIarray[i].Z = GetSumOtkl(mag, ph, freq, i);;
	}
	
	//��������� ���������� ������ ��������
	qsort((void*)CIarray, nZ_cal, sizeof(struct IndZwith_float_Z_str), compare_structs_on_float_Z_and_iZ);
	
	pPHCI[0] = CIarray[0].iZ;
	
	//LowCurveCapIndex = GetCapIndex(CalData[CIarray[0].iZ].C);
	//LowCurveParIndex = GetParIndex(CalData[CIarray[0].iZ].Rp);
	
	if (debug_mode==1)
	{
		printf("\npSortedCurvesDeviation:\n%-6.3f %-6.3f %-6.3f %-6.3f %-6.3f %-6.3f %-6.3f %-6.3f %-6.3f %-6.3f %-6.3f" , CIarray[0].Z, CIarray[1].Z, CIarray[2].Z, CIarray[3].Z, CIarray[4].Z, CIarray[5].Z, CIarray[6].Z, CIarray[7].Z, CIarray[8].Z, CIarray[9].Z, CIarray[10].Z);
		printf("\npSortedCurvesIndexes:\n%-6u %-6u %-6u %-6u %-6u %-6u %-6u %-6u %-6u %-6u %-6u", CIarray[0].iZ, CIarray[1].iZ, CIarray[2].iZ, CIarray[3].iZ, CIarray[4].iZ, CIarray[5].iZ, CIarray[6].iZ, CIarray[7].iZ, CIarray[8].iZ, CIarray[9].iZ, CIarray[10].iZ);
	}

	realloc(CIarray, sizeof(struct IndZwith_float_Z_str)*20);
	
	for (i = 0; i < 20; i++)
	{
		CIarray[i].Z = GetMagOtkl(mag, freq, CIarray[i].iZ);
	}
	//��������� ���������� ������ �������� ���������� ������ ���������
	qsort((void*)CIarray, 20, sizeof(struct IndZwith_float_Z_str), compare_structs_on_float_Z_and_iZ);
	
	for (i = 0; i < 20; i++)
	{
		if (MagBetweenCurves(pPHCI[0],CIarray[i].iZ))
		{
			if (PhBetweenCurves(pPHCI[0],CIarray[i].iZ))
			{
				pPHCI[1] = CIarray[i].iZ;
				break;
			}
		}
	}
	if (i == 20)
	{
		pPHCI[1] = CIarray[1].iZ;
	}
	if (debug_mode==1)
	{
		if (i!=20)
		{
			printf("\nPhase CI might be correct :)");
		}
	}
	
	realloc(CIarray, sizeof(struct IndZwith_float_Z_str)*10);
	pMagCI[0] = CIarray[0].iZ;//����� ������ ������ ����� CIarray[0].iZ
	if (GetCalZ_on_F_iZ (CIarray[0].iZ, freq) < mag)
	{
		for (i = 1; i < 10; i++)
		{
			if (GetCalZ_on_F_iZ (CIarray[i].iZ, freq) > mag)
			{
				pMagCI[1] = CIarray[i].iZ;
				break;
			}
		}
		if (i == 10)
		{
			pMagCI[1] = CIarray[1].iZ;
		}
		}
		else
		{
			for (i = 1; i < 10; i++)
			{
				if (GetCalZ_on_F_iZ (CIarray[i].iZ, freq) < mag)
				{
					pMagCI[1] = CIarray[i].iZ;
					break;
				}
			}
			if (i == 10)
			{
				pMagCI[1] = CIarray[1].iZ;
			}
		}
	if (debug_mode==1)
	{
		if (i!=10)
		{
			printf("\nMag CI might be correct :)");
		}
		else
		{
			printf("\nMag CI calibrating curves are located at one side of measured magnitude of impedance.");
		}
	}
	if (debug_mode==1)
	{
		printf("\npMagSortedCurvesDeviation:\n%-6.3f %-6.3f %-6.3f %-6.3f %-6.3f %-6.3f %-6.3f %-6.3f %-6.3f %-6.3f" , CIarray[0].Z, CIarray[1].Z, CIarray[2].Z, CIarray[3].Z, CIarray[4].Z, CIarray[5].Z, CIarray[6].Z, CIarray[7].Z, CIarray[8].Z, CIarray[9].Z);
		printf("\npMagSortedCurvesIndexes:\n%-6u %-6u %-6u %-6u %-6u %-6u %-6u %-6u %-6u %-6u", CIarray[0].iZ, CIarray[1].iZ, CIarray[2].iZ, CIarray[3].iZ, CIarray[4].iZ, CIarray[5].iZ, CIarray[6].iZ, CIarray[7].iZ, CIarray[8].iZ, CIarray[9].iZ);
	}
	
	if (CorrectIndexesBrute == 1)
	{
		pPHCI[0] = CIarray[FirstCICounter].iZ;
		pPHCI[1] = CIarray[SecondCICounter].iZ;
		
		FirstCICounter++;
		if (FirstCICounter == 10)
		{
			FirstCICounter = 0;
			SecondCICounter++;
			if (SecondCICounter == 10)
			{
				SecondCICounter = 0;
				FirstCICounter = 0;
			}
		}
	}
	free(CIarray);
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
float GetSumOtkl(uint16_t mag, float ph, uint16_t freq, uint16_t iZ)
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
float GetMagOtkl(uint16_t mag, uint16_t freq, uint16_t iZ)
{
	float ret;

	ret = GetCalZ_on_F_iZ (iZ, freq);
	ret = fabs(ret-mag)/sqrtf(mag);
	return ret;
}
float GetPhOtkl(uint16_t ph, uint16_t freq, uint16_t iZ)
{
	float ret;

	ret = GetCalPH_on_F_iZ (iZ, freq);
	ret = fabs(ret-ph)/sqrtf(ph);
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
	result[0] = sqrtf(powf(real,2)+powf(imag,2));															//������ ���������
	result[1] = atanf(imag/real);																							//���� ���������
}

float GetSKOMag(struct IndZwith_float_Z_str * CIarray, uint16_t nZ)
{
	uint16_t i;
	float ret = 0;
	
	for (i=0; i<nZ; i++)
	{
		ret += powf(GetCalZ_on_F_iZ(CIarray[i].iZ, freq) - mag, 2);
	}
	ret /= nZ;
	ret = sqrtf(ret);
	return ret;
}

uint8_t GetCapIndex(uint16_t cap)
{
	uint8_t i;
	for (i=0;i<nCap_cal;i++)
	{
		if (cap == cal_cap_list[i])
			break;
	}
	return i;
}

uint8_t GetParIndex(uint16_t par)
{
	uint8_t i;
	for (i=0;i<nPar_cal;i++)
	{
		if (par == cal_par_list[i])
			break;
	}
	return i;
}

uint8_t MagBetweenCurves(uint16_t iZ1, uint16_t iZ2)
{
	if (GetCalZ_on_F_iZ(iZ1, freq) > mag)
	{
		if (GetCalZ_on_F_iZ(iZ2, freq) < mag)
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		if (GetCalZ_on_F_iZ(iZ2, freq) > mag)
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
}
uint8_t PhBetweenCurves(uint16_t iZ1, uint16_t iZ2)
{
	if (GetCalPH_on_F_iZ(iZ1, freq) > ph)
	{
		if (GetCalPH_on_F_iZ(iZ2, freq) < ph)
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		if (GetCalPH_on_F_iZ(iZ2, freq) > ph)
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
}

// END OF FILE

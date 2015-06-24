#include "math.h"
#include "system.h"

extern uint16_t nZ_cal;

typedef struct {float mag; float ph; uint32_t freq;} mag_ph_calc_calibr_struct_t; // freq in Q8

mag_ph_calc_calibr_struct_t MagPhcT_st;

struct CalData_struct* CalData;
struct CalData_struct* CalDataCalibr;

struct PrepareStructForZcal_str {
	uint8_t		iFmin;
	uint8_t 	iFmax;
	uint16_t	RamSize;
};

// Calibration frequency list in kHz
const uint32_t cal_freq_list[nF_cal] = { 10, 15, 20, 30, 40, 60, 80, 100, 120, 140, 160, 180, 200, 250, 300, 350, 400, 450, 500, 700, 1000,\
																		1400, 1900, 2500, 3200, 4000, 5000 };

extern uint32_t C;
extern uint32_t Rt;
extern uint32_t Rb;
extern uint32_t Rs;
extern uint32_t Rp;
extern uint8_t UART_pressed_enter;
extern char cmdbuf [15];
extern int uart_rcv_len_cnt;
extern int8_t command;
extern void SaveCalData( struct CalData_struct* CalDataClibr, uint8_t stop_bit );
extern void LoadCalData(void);
extern uint8_t debug_mode;//, raw_data_mode = 0, FirstCalStart = 1;

void CalibrateMagPhaseCalcTheoretic(mag_ph_calc_calibr_struct_t *);
struct PrepareStructForZcal_str GetPrepareStructForZcal (void);

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

	temp = 2 * pi * st->freq * Rp * Rp * C * 0.000000000001;
	temp2 = 1 + temp * 2 * pi * st->freq * C * 0.000000000001;
	st->mag = hypotf(Rp / temp2 + Rs, temp / temp2);
	st->ph = - atan((temp / temp2) / (Rp / temp2 + Rs));
}

void Calibrating(void)
{
	static uint16_t CalZCounter;
	struct PrepareStructForZcal_str PrepareStructForZcal;
	static uint8_t AllowSaveFlag=0;
	uint_fast8_t freq_counter;
	uint32_t freq;
	volatile uint32_t temp_freq;
	uint32_t result[2];
	uint32_t mag;
	uint32_t ph;
	uint16_t temp;
	uint8_t temp2;
	static uint8_t UART_cal_state = 0;
	static uint8_t UART_cal_inout = 0;
	
	extern volatile uint32_t DdsFreq;
	
	if (UART_pressed_enter)
	{
		UART_pressed_enter = 0;
		if (UART_cal_state ==0)
		{
			if ((strcmp(cmdbuf, "cap") == 0) && (uart_rcv_len_cnt == 3)) 			UART_cal_state = 1;
			else if ( atoi(cmdbuf) != 0 ) 																		UART_cal_state = 2;
			else if ((strcmp(cmdbuf, "top") == 0) && uart_rcv_len_cnt == 3) 	UART_cal_state = 3;
			else if ((strcmp(cmdbuf, "bot") == 0) && uart_rcv_len_cnt == 3) 	UART_cal_state = 4;
			else if ((strcmp(cmdbuf, "par") == 0) && uart_rcv_len_cnt == 3) 	UART_cal_state = 5;
			else if ((strcmp(cmdbuf, "ser") == 0) && uart_rcv_len_cnt == 3) 	UART_cal_state = 6;
			else if ( uart_rcv_len_cnt == 0)																	UART_cal_state = 7;
			else if ((strcmp(cmdbuf, "stop") == 0) && uart_rcv_len_cnt == 4)	UART_cal_state = 8;
			else if ((strcmp(cmdbuf, "load") == 0) && uart_rcv_len_cnt == 4)	UART_cal_state = 9;
			else if ((strcmp(cmdbuf, "clear") == 0) && uart_rcv_len_cnt == 5)	UART_cal_state = 10;
		}
		if (UART_cal_state == 1)
		{
			if (UART_cal_inout == 0)
			{
				UART_cal_inout = 1;
				printf("\nEnter value of calibrating capacitor (which are serial with calibrating resistor and is located between electrodes):\n>");
			}
			else if (UART_cal_inout == 1)
			{
				UART_cal_inout = 0;
				sscanf(cmdbuf, "%u", &C);
				printf ("\n\rCapacitor value is %d pF.", C);
				printf("\nType next calibrating command.\n>");
				UART_cal_state = 0;
			}
		}
		else if (UART_cal_state == 2)
		{
			sscanf(cmdbuf, "%u", &C);
			printf ("\n\rCapacitor value is %d pF.", C);
			printf("\nType next calibrating command.\n>");
			UART_cal_state = 0;
		}
		else if (UART_cal_state == 3)
		{
			if (UART_cal_inout == 0)
			{
				UART_cal_inout = 1;
				printf("\nEnter top calibrating resistor (which is located in the first electrode):\n>");
			}
			else if (UART_cal_inout == 1)
			{
				UART_cal_inout = 0;
				sscanf(cmdbuf, "%u", &Rt);
				printf ("\nTop resistor value is %d Ohm.", Rt);
				printf("\nType next calibrating command.\n>");
				UART_cal_state = 0;
			}
		}
		else if (UART_cal_state == 4)
		{
			if (UART_cal_inout == 0)
			{
				UART_cal_inout = 1;
				printf("\nEnter bottom calibrating resistor (which is located in the second electrode):\n>");
			}
			else if (UART_cal_inout == 1)
			{
				UART_cal_inout = 0;
				sscanf(cmdbuf, "%u", &Rb);
				printf ("\nBottom resistor value is %d Ohm.", Rb);
				printf("\nType next calibrating command.\n>");
				UART_cal_state = 0;
			}
		}
		else if (UART_cal_state == 5)
		{
			if (UART_cal_inout == 0)
			{
				UART_cal_inout = 1;
				printf("\nEnter parralel calibrating resistor (which are parralel with calibrating capacitor and is located between electrodes):\n>");
			}
			else if (UART_cal_inout == 1)
			{
				UART_cal_inout = 0;
				sscanf(cmdbuf, "%u", &Rp);
				printf ("\nParralel resistor value is %d Ohm.", Rp);
				printf("\nType next calibrating command.\n>");
				UART_cal_state = 0;
			}
		}
		else if (UART_cal_state == 6)
		{
			if (UART_cal_inout == 0)
			{
				UART_cal_inout = 1;
				printf("\nEnter serial calibrating resistor (which are serial with calibrating capacitor and is located between electrodes):\n>");
			}
			else if (UART_cal_inout == 1)
			{
				UART_cal_inout = 0;
				sscanf(cmdbuf, "%u", &Rs);
				printf ("\nSerial resistor value is %d Ohm.", Rs);
				printf("\nType next calibrating command.\n>");
				UART_cal_state = 0;
			}
		}
		else if (UART_cal_state == 7)
		{
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
					// Сохраняем во флеш только если текущая нагрузка релевантна и не было очистки калибровочных
					// данных предидущей нагрузки
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
						ph = result[1]>>4;
						mag = result[0]>>4;
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
			UART_cal_state = 0;
		}
		else if (UART_cal_state == 8)
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
			UART_cal_state = 0;
		}
		else if (UART_cal_state == 9)
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
			UART_cal_state = 0;
		}
		else if (UART_cal_state == 10)
		{
			CalZCounter--;
			AllowSaveFlag = 0;
			printf("\nLast impedance calibration data deleted. Type next command.\n>");
			UART_cal_state = 0;
		}
		else if (UART_cal_state == 0)
		{
			printf("\nWrong command. Please type calibrating command.\n>");
			UART_pressed_enter = 0;
		}
		memset(cmdbuf,0,15);
		uart_rcv_len_cnt=0;
		UART_pressed_enter = 0;
	}
}

/*********************************************************************//**
* @brief        	Функция рассчитывает необходимый объем оперативной памяти для размещения калибровчных данных
									для нагрузки с параметрами C, Rt, Rb, Rs, Rp
	@param[]				none
* @return       	Структура типа PrepareStructForZcal, содержащая индексы минимальной и максимальной частот,
									на которых имеет смысл калибровать и необходимый для каждого из массивов калибровочных данных
									объем ОЗУ в байтах
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
	temp.iFmin = 0;	//Чисто чтоб не было ворнинга
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
		
		//Определение минимальной подходящей частоты
		if ( ( (magnitude < 1500) && (Xc < (200 * Rp) ) ) || (C == 0) )	// if Zc<200*Rp or C=0
		{
			if (first==1)
			{
				first=0;
				temp.iFmin = freq_counter;
			}
			//Определение максимальной подходящей частоты
			if ( ( (magnitude > 150) && (phase > (-60 * 3.1415926 / 180)) &&  (Xc > (((float)(Rp))/10)) ) || (C == 0))			// if ... or C=0
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
	//temp.RamSize += sizeof(Zarray_t); откуда-то взялось
	temp.iFmax = (C == 0) ? nF_cal-1 : temp.iFmax;
	return temp;
}

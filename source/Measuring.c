#include "Measuring.h"

uint32_t mag;
uint32_t ph;
extern uint32_t cal_freq_list[nF_cal];
uint8_t freq_index=0;		//������ ������������� �������, ������� ��������� � freq, �� ������ ���
float f_coef;
extern uint8_t debug_mode;
uint8_t CorrectIndexesBrute = 0;
uint8_t CorrectIndexesOverride;

//uint16_t pCorrectIndexes[2] = {0, 0};
uint16_t pMagCI[2] = {0, 0};
uint16_t pPHCI[2] = {0, 0};

extern uint16_t nZ_cal;

uint16_t FirstOverrideIndex;
uint16_t SecondOverrideIndex;

uint8_t EqualIndexes;

extern struct CalData_struct* CalData;
extern uint32_t C;
extern uint32_t Rt;
extern uint32_t Rb;
extern uint32_t Rs;
extern uint32_t Rp;

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
	
	ph = result[1]>>4;
	mag = result[0]>>4;
	
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

	
	void GetCorrectIndexes(uint16_t* pMagCI, uint16_t* pPHCI, uint16_t freq, uint32_t mag, uint32_t ph)
{
	uint16_t i;
	
	struct IndZwith_float_Z_str * CIarray;
	
	static uint8_t FirstCICounter = 0, SecondCICounter = 0;
	
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
		if (MagBetweenCurves(pPHCI[0],CIarray[i].iZ, freq))
		{
			if (PhBetweenCurves(pPHCI[0],CIarray[i].iZ, freq))
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

uint8_t MagBetweenCurves(uint16_t iZ1, uint16_t iZ2, uint16_t freq)
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
uint8_t PhBetweenCurves(uint16_t iZ1, uint16_t iZ2, uint16_t freq)
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

float GetRealPH_on_F_iZ1_iZ2_for_PH(uint16_t freq, uint16_t iZ1, uint16_t iZ2, uint32_t PH)
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

float GetRealZ_on_F_iZ1_iZ2_for_Z(uint16_t freq, uint16_t iZ1, uint16_t iZ2, uint32_t Z)
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

/*********************************************************************//**
* @brief        	������� ������������ ����� ������� ���������� � ���������/100 ������ ����������� ��������� � ���
									���� �� ������������� ������������� ������ � �������� iZ
	@param[mag]			���������� ������ ���������
	@param[ph]			���������� ���� ���������
	@param[freq]		������� ������� DDS, ��� (uint16_t). ������ ���� ������ ������� �������, �� ������� ������������� ����������
	@param[iZ]		������ ������������� ��������
* @return       	����� ������� ���������� (float).
**********************************************************************/
float GetSumOtkl(uint32_t mag, uint32_t ph, uint16_t freq, uint16_t iZ)
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
float GetMagOtkl(uint32_t mag, uint16_t freq, uint16_t iZ)
{
	float ret;

	ret = GetCalZ_on_F_iZ (iZ, freq);
	ret = fabs(ret-mag)/sqrtf(mag);
	return ret;
}
float GetPhOtkl(uint32_t ph, uint16_t freq, uint16_t iZ)
{
	float ret;

	ret = GetCalPH_on_F_iZ (iZ, freq);
	ret = fabs(ret-ph)/sqrtf(ph);
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

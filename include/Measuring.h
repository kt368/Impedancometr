#include "System.h"
#include "Flash_cal_adresses.h"

int compare_structs_on_uint_Z_and_iZ(const void *arg1, const void *arg2);
int compare_structs_on_float_Z_and_iZ(const void *arg1, const void *arg2);

void Measure(float results[2], uint16_t freq);

void GetCorrectIndexes(uint16_t* pMagCI, uint16_t* pPHCI, uint16_t freq, uint32_t mag, uint32_t ph);
float GetSumOtkl(uint32_t mag, uint32_t ph, uint16_t freq, uint16_t iZ);
float GetMagOtkl(uint32_t mag, uint16_t freq, uint16_t iZ);
float GetPhOtkl(uint32_t ph, uint16_t freq, uint16_t iZ);

uint16_t Calc_iZ_for_Min_Z_OnCur_iF (uint8_t freq_index);
uint16_t Calc_iZ_for_Max_Z_OnCur_iF (uint8_t freq_index);

uint8_t MagBetweenCurves(uint16_t iZ1, uint16_t iZ2, uint16_t freq);
uint8_t PhBetweenCurves(uint16_t iZ1, uint16_t iZ2, uint16_t freq);

float GetCalZ_on_F_iZ (uint16_t iZ, uint16_t freq);
float GetCalPH_on_F_iZ (uint16_t iZ, uint16_t freq);

float GetRealZ_on_F_iZ1_iZ2_for_Z(uint16_t freq, uint16_t iZ1, uint16_t iZ2, uint32_t Z);
float GetRealPH_on_F_iZ1_iZ2_for_PH(uint16_t freq, uint16_t iZ1, uint16_t iZ2, uint32_t PH);

void GetRealCalMagPh(float result[2], uint16_t freq, uint16_t iZ);

uint32_t GetCalZ_on_iZ_iF (uint16_t iZ, uint8_t iF);

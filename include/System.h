#include "lpc17xx_gpio.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_systick.h"
#include <LPC17XX.h>                       /* LPC17XX definitions         */
#include <system_LPC17xx.h>                /* LPC17XX definitions         */
#include "lpc17xx_pinsel.h"
#include "pt.h"														 /* PROTOTHREADS sources */
#include "lpc17xx_libcfg_default.h"
#include "math.h"
#include "GPIO_signals.h"
#include "Global_header.h"
#include <stdio.h>
#include <stdlib.h>
#include "AD9833.h"
#include <string.h>
#include "lpc17xx_i2s.h"

#define LOW									0
#define HIGH								1
#define nF_cal							27
#define nZ_cal_max					1000

#define CalData_var					1

//Definitions of UART commands

#define Calibrate	7
#define f					9
#define BADKEY		-1
#define	none			0

//Definitions of UART state
#define enter			1

#define UART    LPC_UART0
#define CNTLQ      0x11
#define CNTLS      0x13
#define DEL        0x7F
#define BACKSPACE  0x08
#define CR         0x0D
#define LF         0x0A

#define MAGNITUDE 0
#define PHASE 1

#define stop_saving 1
#define nonstop_saving 0

#define max_mag_where_phase_influence 0.43 // In percents of max ADC output value

// Types definitions

typedef struct {float mag; float ph; uint32_t freq;} mag_ph_calc_calibr_struct_t; // freq in Q8
typedef struct {uint32_t mag; uint32_t ph;} Zarray_t;

struct CalData_struct {	//Size 8*32 bit = 8*4 bytes = 32 byte
	uint32_t	nFmin;			//Индекс первой калибровочной частоты
	uint32_t	nFmax;			//Индекс +1 максимальной калибровочной частоты.
	uint32_t	C;					//Калибровочный конденсатор (в пФ).
	uint32_t	R;					//Калибровочный резистор, паралельный конденсатору (в Ом).
	Zarray_t 	*Zarray;		//pointer to dynsmic size array of structs (n not necessarily equal nF)
												//We have nFmax - nFmin sets of calibrating coefficients
};

struct IndZwith_uint_Z_str {uint16_t Z; uint8_t iZ;};
struct IndZwith_float_Z_str {float Z; uint8_t iZ;};

// Functions definitions

void Init_GPIO_Pins (void);
void Init (void);
void Init_ADC (void);
void test (void);
void led_on (void);
void led_off (void);
void V12SW_ON2(void);
void CalibrateMagPhaseCalcTheoretic(mag_ph_calc_calibr_struct_t *);
void logspace(uint32_t base, uint8_t n_bins, uint32_t min, uint32_t max, uint32_t array[]);
uint16_t CalcCalibrDataSize(void);
uint8_t Calc_iZ_for_Min_Z_OnCur_iF (uint8_t freq_index);
uint8_t Calc_iZ_for_Max_Z_OnCur_iF (uint8_t freq_index);
void SortZIndOnCurF (struct IndZwith_uint_Z_str * pIndZwith_uint_Z, uint16_t freq);
struct PrepareStructForZcal_str {
	uint8_t		iFmin;
	uint8_t 	iFmax;
	uint16_t	RamSize;
};


int compare_structs_on_uint_Z_and_iZ(const void *arg1, const void *arg2);
int compare_structs_on_float_Z_and_iZ(const void *arg1, const void *arg2);

void Measure(float results[2], uint16_t freq);

void GetCorrectIndexes(uint8_t* pCorrectIndexes, uint16_t freq, float mag, float ph);
float GetSumOtkl(float mag, float ph, uint16_t freq, uint8_t n_cal);

uint32_t GetCalZ_on_iZ_iF (uint8_t iZ, uint8_t iF);
uint32_t GetCalPH_on_iZ_iF (uint8_t iZ, uint8_t iF);

float GetCalZ_on_F_iZ (uint8_t iZ, uint16_t freq);
float GetCalPH_on_F_iZ (uint8_t iZ, uint16_t freq);

float GetRealZ_on_F_iZ_for_Z(uint16_t freq, uint8_t iZ, uint16_t Z);
float GetRealZ_on_F_iZ1_iZ2_for_Z(uint16_t freq, uint8_t iZ1, uint8_t iZ2, uint16_t Z);
float GetRealZ_on_iF_iZ_for_Z (uint8_t freq_index, uint16_t Z, uint8_t iZ);

float GetRealPH_on_F_iZ_for_Z(uint16_t freq, uint8_t iZ, uint16_t PH);
float GetRealPH_on_iF_iZ_for_PH (uint8_t freq_index, uint16_t PH, uint8_t iZ);

void wait (uint32_t t);
struct PrepareStructForZcal_str GetPrepareStructForZcal (void);

extern void AD9833_SPI_Init(void);
extern void AD9833_SetFreq(uint32_t freq);
extern void AD9833_SetPhase(uint16_t phase);
extern void AD9833_Stop(void);
extern void AD9833_Start(void);

extern void __enable_irq(void);
extern int __disable_irq(void);

PT_THREAD(Starting(struct pt *pt));

uint16_t ADC_int_RUN(LPC_ADC_TypeDef *ADCx, ADC_CHANNEL_SELECTION channel, uint8_t n_Samples, uint8_t overs_bits);
uint32_t ADC_RUN(uint8_t n_Samples);

void SaveCalData( struct CalData_struct* CalDataClibr, uint8_t stop_bit );

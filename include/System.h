#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"
#include "GPIO_signals.h"
#include <stdio.h>
#include "AD7793.h"				// AD7793 definitions.
#include "AD9833.h"				// AD9833 definitions.
#include <string.h>
#include <stdlib.h>


#define nF_cal							27


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

#define nonstop_saving 0
#define stop_saving 1

#define nZ_cal_max					1000

typedef struct {uint32_t mag; uint32_t ph;} Zarray_t;

struct CalData_struct {	//Size 8*32 bit = 8*4 bytes = 32 byte
	uint32_t	nFmin;			//������ ������ ������������� �������
	uint32_t	nFmax;			//������ +1 ������������ ������������� �������.
	uint32_t	C;					//������������� ����������� (� ��).
	uint32_t	Rp;					//������������� ��������, ����������� ������������ (� ��).
	uint32_t	Rs;					//������������� ��������, ��������������� � ������������� (� ��).
	uint32_t	Rt;					//������������� ��������, ����� ����� �������� ����������� (� ��).
	uint32_t	Rb;					//������������� ��������, ����� ����� ������� ����������� (� ��).
	Zarray_t 	*Zarray;		//pointer to dynsmic size array of structs (n not necessarily equal nF)
												//We have nFmax - nFmin sets of calibrating coefficients
};

// Functions definitions

void Init_GPIO_Pins (void);
void Init (void);
void test (void);
void blink (void);
void wait(uint32_t t);

void ADC_RUN(uint32_t *result);

void Calibrating(void);

struct IndZwith_float_Z_str {float Z; uint16_t iZ;};

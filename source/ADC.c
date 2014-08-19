#include "ADC.h"

/* I2S ADC PCM4202  */

/* ADC - slave mode, MCU - master mode*/

/*
Используемые пины:
I2SRX_SDA	DATA	64
I2STX_CLK	BCK		63
I2STX_WS	LRCK	62
*/

/*
Frequencies:
System clock: 25 MHz
Sampling frequency: Fs = 25 MHz / 384 = 65104.167 sps
F_BCKI = 2 * Fs * 32 = 5.1667 MHz
HZ!!!!!!!!!!!!     I2STXBITRATE[5:0] должен быть равен 64 HZ!!!!!!!!!!!!!!!!!!

*/
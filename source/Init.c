/**************************************************************************//**
 * @file     Init.c
 * @brief    System initialization file
 * @note
 *
******************************************************************************/
#include "Init.h"                       /* Init file header */

PINSEL_CFG_Type PinCfg;

	void Init(void)
	{
		
		V12SW_OFF;
		Init_GPIO_Pins();
		V12SW_ON;
		BatCharge_OFF;								
		//Switch OFF battarey charging
		
		DDS_FSYNC_HIGH;							
		//Prevent loading SPI data in DDS AD9833
		
		Vbus_sys_SW_OFF;							
		//Disconnect USB Vbus from V6 system power bus
		
		VLED_SW_LOW;								
		//Low LCD LED brightless
		
		SCRN_SEL_NO;								
		//Not transfer data to display (through SPI)
		
		Vbat_supply_ON;								
	//receive power from battarey through main switch
	
		SER_Init(115200);
		Init_AD7793();
		test();
	}

	void Init_AD7793(void)
	{
		uint8_t AD7793_Init_result;
		AD7793_Init_result = AD7793_Init();
		if (AD7793_Init_result == 0)
		{
			printf("\n AD7793 Init Error\n>");
		}

		AD7793_SetMode(AD7793_MODE_IDLE);
		AD7793_SetClk(AD7793_CLK_INT);
		AD7793_SetRate(0x6);
		AD7793_SetBias(0x0);
		AD7793_SetBO(0);
		AD7793_SetUnB(1);
		AD7793_SetBoost(0);
		AD7793_SetGain(AD7793_GAIN_1);
		AD7793_SetIntReference(AD7793_REFSEL_EXT);
		AD7793_SetBuff(0);
		AD7793_SetChannel(AD7793_CH_AIN1P_AIN1M);
		AD7793_Calibrate(AD7793_MODE_CAL_INT_ZERO, AD7793_CH_AIN1P_AIN1M);
		AD7793_Calibrate(AD7793_MODE_CAL_INT_FULL, AD7793_CH_AIN1P_AIN1M);
		
	}
	
	void Init_GPIO_Pins (void)
	{
	/* P1.20 and P2.10 are input with pullup (after reset)*/
	/*
	* Init Vbus detect pin on P0.0
	*/
	PinCfg.Funcnum = PINSEL_FUNC_0;
	PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	PinCfg.Pinnum = PINSEL_PIN_0;
	PinCfg.Portnum = PINSEL_PORT_0;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(0, (1<<0), 1); //Set P0.0 as output!!!!!!!!!!!!!!!!11input

	/*
	* Init DDS_FSYNC signal on P0.1
	*/
	PinCfg.Funcnum = PINSEL_FUNC_0;
	PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	PinCfg.Pinnum = PINSEL_PIN_1;
	PinCfg.Portnum = PINSEL_PORT_0;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetValue(0, 1<<1);			//Set P0.1 high
	GPIO_SetDir(0, (1<<1), 1); //Set P0.1 as output
	
	/*
	* Init UART_TX signal on P0.2
	*/
	PinCfg.Funcnum = PINSEL_FUNC_1;
	PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	PinCfg.Pinnum = PINSEL_PIN_2;
	PinCfg.Portnum = PINSEL_PORT_0;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(0, (1<<2), 1); //Set P0.2 as output
	
	/*
	* Init UART_RX signal on P0.3
	*/
	PinCfg.Funcnum = PINSEL_FUNC_1;
	PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	PinCfg.Pinnum = PINSEL_PIN_3;
	PinCfg.Portnum = PINSEL_PORT_0;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(0, (1<<3), 0); //Set P0.3 as input
		
	/*
	* Init D/C signal on P0.9
	*/
	PinCfg.Funcnum = PINSEL_FUNC_0;
	PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	PinCfg.Pinnum = PINSEL_PIN_9;
	PinCfg.Portnum = PINSEL_PORT_0;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(0, (1<<9), 1); //Set P0.9 as output

	/*
	* Init Vbus_sys_sw signal on P0.10
	*/
	PinCfg.Funcnum = PINSEL_FUNC_0;
	PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	PinCfg.Pinnum = PINSEL_PIN_10;
	PinCfg.Portnum = PINSEL_PORT_0;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(0, (1<<10), 1); //Set P0.10 as output

	/*
	* Init SCK signal on P0.15
	*/
	PinCfg.Funcnum = PINSEL_FUNC_3;
	PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	PinCfg.Pinnum = PINSEL_PIN_15;
	PinCfg.Portnum = PINSEL_PORT_0;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(0, (1<<15), 1); //Set P0.15 as output

	/*
	* Init MISO signal on P0.17
	*/
	PinCfg.Funcnum = PINSEL_FUNC_3;
	PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	PinCfg.Pinnum = PINSEL_PIN_17;
	PinCfg.Portnum = PINSEL_PORT_0;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(0, (1<<17), 0); //Set P0.17 as input
	
	/*
	* Init MOSI signal on P0.18
	*/
	PinCfg.Funcnum = PINSEL_FUNC_3;
	PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	PinCfg.Pinnum = PINSEL_PIN_18;
	PinCfg.Portnum = PINSEL_PORT_0;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(0, (1<<18), 1); //Set P0.18 as output
	
		/*
	* Init V_lcd_led_SW signal on P0.25
	*/
	PinCfg.Funcnum = PINSEL_FUNC_0;
	PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	PinCfg.Pinnum = PINSEL_PIN_25;
	PinCfg.Portnum = PINSEL_PORT_0;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(0, (1<<25), 1); //Set P0.25 as output

/*
	* Init Vbat_ADC signal on P0.26
	*/
	PinCfg.Funcnum = PINSEL_FUNC_1;
	PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	PinCfg.Pinnum = PINSEL_PIN_26;
	PinCfg.Portnum = PINSEL_PORT_0;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(0, (1<<26), 0); //Set P0.26 as input
	
	/*
	* Init D_P signal on P0.29
	*/
	PinCfg.Funcnum = PINSEL_FUNC_1;
	PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	PinCfg.Pinnum = PINSEL_PIN_29;
	PinCfg.Portnum = PINSEL_PORT_0;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(0, (1<<29), 0); //Set P0.29 as input
	
	/*
	* Init D_N signal on P0.30
	*/
	PinCfg.Funcnum = PINSEL_FUNC_1;
	PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	PinCfg.Pinnum = PINSEL_PIN_30;
	PinCfg.Portnum = PINSEL_PORT_0;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(0, (1<<30), 0); //Set P0.30 as input
	
	/*
	* Init Vbat_supply signal on P1.4
	*/
	PinCfg.Funcnum = PINSEL_FUNC_0;
	PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	PinCfg.Pinnum = PINSEL_PIN_4;
	PinCfg.Portnum = PINSEL_PORT_1;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(1, (1<<4), 1); //Set P1.4 as output
	
	/*
	* Init X- signal on P1.8
	*/
	PinCfg.Funcnum = PINSEL_FUNC_0;
	PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	PinCfg.Pinnum = PINSEL_PIN_8;
	PinCfg.Portnum = PINSEL_PORT_1;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(1, (1<<8), 0); //Set P1.8 as input
	
	/*
	* Init Y- signal on P1.9
	*/
	PinCfg.Funcnum = PINSEL_FUNC_0;
	PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	PinCfg.Pinnum = PINSEL_PIN_9;
	PinCfg.Portnum = PINSEL_PORT_1;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(1, (1<<9), 0); //Set P1.9 as input
	
	/*
	* Init USB_UP_LED signal on P1.18
	*/
	PinCfg.Funcnum = PINSEL_FUNC_1;
	PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	PinCfg.Pinnum = PINSEL_PIN_18;
	PinCfg.Portnum = PINSEL_PORT_1;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(1, (1<<18), 0); //Set P1.18 as input
	
	/*
	* Init V7_bat_SW signal on P1.19
	*/
	PinCfg.Funcnum = PINSEL_FUNC_0;
	PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	PinCfg.Pinnum = PINSEL_PIN_19;
	PinCfg.Portnum = PINSEL_PORT_1;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(1, (1<<19), 1); //Set P1.19 as output
	
	/*
	* Init V12_SW signal on P1.22
	*/
	PinCfg.Funcnum = PINSEL_FUNC_0;
	PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	PinCfg.Pinnum = PINSEL_PIN_22;
	PinCfg.Portnum = PINSEL_PORT_1;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(1, (1<<22), 1); //Set P1.22 as output
	
	/*
	* Init VLED_SW signal on P1.23
	*/
	PinCfg.Funcnum = PINSEL_FUNC_0;
	PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	PinCfg.Pinnum = PINSEL_PIN_23;
	PinCfg.Portnum = PINSEL_PORT_1;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(1, (1<<23), 1); //Set P1.23 as output
	
	/*
	* Init nCS signal on P1.29
	*/
	PinCfg.Funcnum = PINSEL_FUNC_0;
	PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	PinCfg.Pinnum = PINSEL_PIN_29;
	PinCfg.Portnum = PINSEL_PORT_1;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetValue(1, 1<<29);
	GPIO_SetDir(1, (1<<29), 1); //Set P1.29 as output
	
	/*
	* Init Y+ signal on P1.30
	*/
	PinCfg.Funcnum = PINSEL_FUNC_0;
	PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	PinCfg.Pinnum = PINSEL_PIN_30;
	PinCfg.Portnum = PINSEL_PORT_1;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(1, (1<<30), 0); //Set P1.30 as input
	
	/*
	* Init X+ signal on P1.31
	*/
	PinCfg.Funcnum = PINSEL_FUNC_0;
	PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	PinCfg.Pinnum = PINSEL_PIN_31;
	PinCfg.Portnum = PINSEL_PORT_1;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(1, (1UL<<31), 0); //Set P1.31 as input
	
	/*
	* Init SCRN_SEL signal on P2.8
	*/
	PinCfg.Funcnum = PINSEL_FUNC_0;
	PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	PinCfg.Pinnum = PINSEL_PIN_8;
	PinCfg.Portnum = PINSEL_PORT_2;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(2, (1<<8), 1); //Set P2.8 as output
	
	/*
	* Init USB_CONNECT signal on P2.9
	*/
	PinCfg.Funcnum = PINSEL_FUNC_1;
	PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	PinCfg.Pinnum = PINSEL_PIN_9;
	PinCfg.Portnum = PINSEL_PORT_2;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(2, (1<<9), 1); //Set P2.9 as output
	
	}
/* --------------------------------- End Of File ------------------------------ */

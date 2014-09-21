/**************************************************************************//**
 * @file     Init.c
 * @brief    System initialization file
 * @note
 *
******************************************************************************/
#include "Init.h"                       /* Init file header */
#define fs 65104 //65107 65105

PINSEL_CFG_Type PinCfg;

	void Init(void)
	{
		extern uint32_t cal_freq_list[];
		
		//SYSTICK_InternalInit(10);				//Initialize SysTIck counter for 5 ms ticks
		//SYSTICK_Cmd(ENABLE);						

//Enable SysTick counter
		//NVIC_SetPriority (SysTick_IRQn, (1<<__NVIC_PRIO_BITS) - 1);
		//SYSTICK_IntCmd(ENABLE);				//Enable SysTick counter interrupts
		V12SW_OFF;
		Init_GPIO_Pins();
		Init_ADC();
		test();
		V12SW_ON2();
		test();
		
		I2S_Init(LPC_I2S);
		
		I2S_FreqConfig(LPC_I2S, fs, I2S_RX_MODE);
		I2S_FreqConfig(LPC_I2S, fs, I2S_TX_MODE);
		
		I2S_MODEConf.clksel = I2S_CLKSEL_FRDCLK;
		I2S_MODEConf.fpin = I2S_4PIN_DISABLE;
		I2S_MODEConf.mcena = I2S_MCLK_DISABLE;
		I2S_ModeConfig(LPC_I2S, &I2S_MODEConf, I2S_TX_MODE);
		
		I2S_MODEConf.clksel = I2S_CLKSEL_FRDCLK;
		I2S_MODEConf.fpin = I2S_4PIN_ENABLE;
		I2S_MODEConf.mcena = I2S_MCLK_DISABLE;
		I2S_ModeConfig(LPC_I2S, &I2S_MODEConf, I2S_RX_MODE);
		
		ConfigStruct.wordwidth = I2S_WORDWIDTH_32;
		ConfigStruct.mono = I2S_STEREO;
		ConfigStruct.stop = I2S_STOP_DISABLE;
		ConfigStruct.reset = I2S_RESET_DISABLE;
		ConfigStruct.ws_sel = I2S_MASTER_MODE;
		ConfigStruct.mute = I2S_MUTE_DISABLE;
		I2S_Config(LPC_I2S, I2S_RX_MODE, &ConfigStruct);
		
		ConfigStruct.wordwidth = I2S_WORDWIDTH_32;
		ConfigStruct.mono = I2S_STEREO;
		ConfigStruct.stop = I2S_STOP_DISABLE;
		ConfigStruct.reset = I2S_RESET_DISABLE;
		ConfigStruct.ws_sel = I2S_MASTER_MODE;
		ConfigStruct.mute = I2S_MUTE_DISABLE;
		I2S_Config(LPC_I2S, I2S_TX_MODE, &ConfigStruct);
		
		I2S_IRQConfig(LPC_I2S, I2S_RX_MODE, 8);

		I2S_FreqConfig(LPC_I2S, fs, I2S_RX_MODE);
		I2S_FreqConfig(LPC_I2S, fs, I2S_TX_MODE);

		
		SER_Init(115200);
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

		LPC_I2S -> I2SDAI |= 0x18;
		LPC_I2S -> I2SDAO |= 0x18;
		LPC_I2S -> I2SDAI &= ~0x18;
		LPC_I2S -> I2SDAO &= ~0x18;
		
		NVIC_SetPriority(I2S_IRQn,0);
		NVIC_SetPriority(UART0_IRQn,30);
		NVIC_ClearPendingIRQ(I2S_IRQn);
		I2S_IRQCmd(LPC_I2S, I2S_RX_MODE, ENABLE);
		NVIC_EnableIRQ(I2S_IRQn);
		
	}

	void Init_ADC(void)
	{
		/* Configuration for ADC :
    *  ADC conversion rate = 200KHz
    */
    ADC_Init(LPC_ADC, 200000);
    ADC_ChannelCmd(LPC_ADC,2,ENABLE);
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
	* Init I2SRX_SDA(DATA) signal on P0.6
	*/
	PinCfg.Funcnum = PINSEL_FUNC_1;
	PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	PinCfg.Pinnum = PINSEL_PIN_6;
	PinCfg.Portnum = PINSEL_PORT_0;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(0, (1<<6), 0); //Set P0.6 as input
	
	/*
	* Init I2STX_CLK(BCK) signal on P0.7
	*/
	PinCfg.Funcnum = PINSEL_FUNC_1;
	PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	PinCfg.Pinnum = PINSEL_PIN_7;
	PinCfg.Portnum = PINSEL_PORT_0;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(0, (1<<7), 1); //Set P0.7 as output
	
	/*
	* Init I2STX_WS(LRCK) signal on P0.8
	*/
	PinCfg.Funcnum = PINSEL_FUNC_1;
	PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	PinCfg.Pinnum = PINSEL_PIN_8;
	PinCfg.Portnum = PINSEL_PORT_0;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(0, (1<<8), 1); //Set P0.8 as output
	
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
	* Init DATA signal on P0.11
	*/
	PinCfg.Funcnum = PINSEL_FUNC_0;
	PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	PinCfg.Pinnum = PINSEL_PIN_11;
	PinCfg.Portnum = PINSEL_PORT_0;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(0, (1<<11), 0); //Set P0.11 as input

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
	* Init ADC_corr signal on P0.25
	*/
	PinCfg.Funcnum = PINSEL_FUNC_1;
	PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	PinCfg.Pinnum = PINSEL_PIN_25;
	PinCfg.Portnum = PINSEL_PORT_0;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(0, (1<<25), 0); //Set P0.25 as input

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

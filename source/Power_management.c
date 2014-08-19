#include "Power_management.h"
LOW_HIGH_TYPE Vbat;
uint8_t start_adc_bat_bit;
LOW_HIGH_TYPE bat_status;
extern volatile uint32_t systick_ten_millis;
#define ADC_CHANNEL        ADC_CHANNEL_3

PT_THREAD(Power_mgmt(struct pt *pt))
{
	static uint32_t power_mgmt_time_snapshot=0;
	static uint8_t adc_bat_time_reached=0;
	PT_BEGIN(pt);
	/* set adc_bat_time_reached to one every 1 sec */
	if ( (systick_ten_millis - power_mgmt_time_snapshot >= 100) )  //if 10ms*100 time left
	{
		power_mgmt_time_snapshot = systick_ten_millis;
		adc_bat_time_reached = 1;
	}
	/* set adc_bat_time_reached to one every 1 sec */
	
	if (start_adc_bat_bit || adc_bat_time_reached)		//Recieve battarey status: does Vbat>3.7V
	{
		bat_status = ADC_bat();
		if (start_adc_bat_bit==1)
		{
			adc_bat_done;
		}
		if (adc_bat_time_reached==1)
		{
			adc_bat_time_reached=0;		//Sbros adc_bat_time_reached
		}
	}
	
	if (bat_status == LOW)
	{
		V12SW_OFF;
		//Vivesti na display nadpis'
		power_mgmt_time_snapshot=systick_ten_millis;
		PT_WAIT_UNTIL(pt, (power_mgmt_time_snapshot-systick_ten_millis>500) );//Block thread for 5 sec
		VLED_SW_LOW;										//Low LCD LED brightless
		// block all threads, go to the powerdown mode, run potok proverki Vbus
	}

	PT_END(pt);
}

LOW_HIGH_TYPE ADC_bat(void)
{
	uint32_t Vbat_ADC;
	Vbat_ADC = ADC_RUN(LPC_ADC, ADC_CHANNEL, 5, 0);
	Vbat_ADC = ( Vbat_ADC << 15 ) / 621;
	//Vbat_ADC now in Q15 format
	if (Vbat_ADC < battarey_low)
	{
		return LOW;
	}
	else
	{
		return HIGH;
	}
}
// END OF FILE

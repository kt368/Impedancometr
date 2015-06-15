

mag_ph_calc_calibr_struct_t MagPhcT_st;

/*********************************************************************//**
-* @brief        Calculate magnitude and phase of calculated impedance from values, which were
-								inputted throught UART
-* @param[freq]	Current frequency, on which impedance must calculated, in Herz
-* @return       none
-**********************************************************************/
-	
-	void CalibrateMagPhaseCalcTheoretic(mag_ph_calc_calibr_struct_t *st)
-	{
-		float	temp, temp2;
-		float pi = 3.1415926;
-		led_on();
-		temp = 2 * pi * st->freq * Rp * Rp * C * 0.000000000001;
-		temp2 = 1 + temp * 2 * pi * st->freq * C * 0.000000000001;
-		st->mag = hypotf(Rp / temp2 + Rs, temp / temp2);
-		st->ph = - atan((temp / temp2) / (Rp / temp2 + Rs));
-		led_off();
-	}
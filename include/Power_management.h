#include "ProtoThreads.h"
#include "Global_header.h"

#define battarey_low	121241


uint8_t start_adc_bat;

typedef enum
{
	LOW,
	HIGH
}LOW_HIGH_TYPE;

/* Public Functions ----------------------------------------------------------- */
/** @defgroup SPI_Public_Functions SPI Public Functions
 * @{
 */


LOW_HIGH_TYPE ADC_bat(void);							// Measure battarey voltage, compare it to battarey_low

/**
 * @}
 */

/* --------------------------------- End Of File ------------------------------ */

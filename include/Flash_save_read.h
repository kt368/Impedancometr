//#include <LPC17XX.h>
#include "System.h"

#define DISABLEIRQ __disable_irq();
#define ENABLEIRQ  __enable_irq();
#define IAP_LOCATION 0x1FFF1FF1

void CheckAndStoreCalData(uint8_t* StoreIndex, uint8_t* buffer_pointer, uint32_t* buffer, uint32_t* WSECTOR_STARTADDR, uint8_t* WSECTOR_NUM, uint16_t size);
void SaveCalData( struct CalData_struct* CalDataClibr, uint8_t stop_bit );
void LoadCalData(void);

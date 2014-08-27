#include "Flash_save_read.h"
#include "Flash_cal_adresses.h"

//static uint8_t mSectorMemory1[SECTOR_SIZE] __attribute__((at(SectorWithCalStructStartaddr)));
// IAP function
typedef void (*IAP)(unsigned int [], unsigned int []);
static IAP mIAPEntry = (IAP)IAP_LOCATION;

extern struct CalData_struct *CalData;
extern uint32_t cal_freq_list[nF_cal];

//ѕрисвоение значений первого сектора с массивами Zarray и PHarray
uint8_t SectorWithArrayCounter = SectorWithFirstArrayNum;
uint32_t SectorWithArrayStartaddr = SectorWithFirstArrayStartaddr;

extern char cmdbuf [15];
extern int uart_rcv_len_cnt;
uint32_t FlashCalStructStartaddr = FlashCalStructStartaddr_def;
uint8_t SectorWithCalStructNum = SectorWithCalStructNum_def;
uint32_t SectorWithCalStructStartaddr = SectorWithCalStructStartaddr_def;

// начина€ с адреса SectorWithCalStructStartaddr записываетс€ массив структур CalDataStr, в него вписываютс€ указатели на массивы
// Zarray и PHarray, наход€щиес€ начина€ с адресса SectorWithArrayStartaddr. ћассивы занимают до двух секторов.
void SaveCalData( struct CalData_struct* CalDataClibr, uint8_t stop_bit )
{
	static uint32_t BufferWithArrays[128] __attribute__((at(0x10000100)));
	static uint32_t BufferWithStructs[128] __attribute__((at(0x10000300)));
	static uint8_t buffer_pointer = 0, Structs_descr_pointer = 0;
	
	static uint32_t CalDataFlashAddress;
	
	static uint16_t StoredZCounter = 0;
	uint8_t temp;
	
	uint8_t CalFCounter;
	
	static uint8_t StoreIndexArray=0, StoreIndex=0;
	unsigned int Command[5], Result[5];
	if (StoredZCounter == 0) // ≈сли функци€ SaveCalData вызываетс€ первый раз
	{
		// ѕодготавливаем и очищаем сектор с калибровочными структурами:
		// prepare sector
		Command[0] = 50;
		Command[1] = SectorWithCalStructNum;
		Command[2] = SectorWithCalStructNum;
		DISABLEIRQ;
		mIAPEntry(Command, Result);
		ENABLEIRQ;
		
		// earase sector
		Command[0] = 52;
		Command[1] = SectorWithCalStructNum;
		Command[2] = SectorWithCalStructNum;
		Command[3] = SystemCoreClock / 1000;
		DISABLEIRQ;
		mIAPEntry(Command, Result);
		ENABLEIRQ;
		
		// сохран€ем адресс, во флеш-пам€ти куда сохранитс€ массив Zarray
		CalDataFlashAddress = SectorWithArrayStartaddr;
		
		// prepare sector for storing arrays
		Command[0] = 50;
		Command[1] = SectorWithArrayCounter;
		Command[2] = SectorWithArrayCounter;
		DISABLEIRQ;
		mIAPEntry(Command, Result);
		ENABLEIRQ;
		
		// earase sector
		Command[0] = 52;
		Command[1] = SectorWithArrayCounter;
		Command[2] = SectorWithArrayCounter;
		Command[3] = SystemCoreClock / 1000;
		DISABLEIRQ;
		mIAPEntry(Command, Result);
		ENABLEIRQ;
		
	}

	//—охранение во флеш массива Zarray дл€ текущей калибровочной нагрузки
	for (CalFCounter = 0; CalFCounter < ( CalDataClibr->nFmax - CalDataClibr->nFmin ); CalFCounter++)
	{
		BufferWithArrays[buffer_pointer] = *(uint32_t*)&(CalDataClibr->Zarray[CalFCounter].k);
		buffer_pointer++;
		CheckAndStoreCalData(&StoreIndexArray, &buffer_pointer, BufferWithArrays, &SectorWithArrayStartaddr, &SectorWithArrayCounter, 512);
		BufferWithArrays[buffer_pointer] = *(uint32_t*)&(CalDataClibr->Zarray[CalFCounter].b);
		buffer_pointer++;
		CheckAndStoreCalData(&StoreIndexArray, &buffer_pointer, BufferWithArrays, &SectorWithArrayStartaddr, &SectorWithArrayCounter, 512);
		BufferWithArrays[buffer_pointer] = *(uint32_t*)&(CalDataClibr->Zarray[CalFCounter].Zmin);
		buffer_pointer++;
		CheckAndStoreCalData(&StoreIndexArray, &buffer_pointer, BufferWithArrays, &SectorWithArrayStartaddr, &SectorWithArrayCounter, 512);
	}
	//—охранение во флеш массива PHarray дл€ текущей калибровочной нагрузки с номером CalZCounter (в текущем запуске функции SaveCalData)
	for (CalFCounter = 0; CalFCounter < ( CalDataClibr->nFmax - CalDataClibr->nFmin ); CalFCounter++)
	{
		BufferWithArrays[buffer_pointer] = *(uint32_t*)&(CalDataClibr->PHarray[CalFCounter].k);
		buffer_pointer++;
		CheckAndStoreCalData(&StoreIndexArray, &buffer_pointer, BufferWithArrays, &SectorWithArrayStartaddr, &SectorWithArrayCounter, 512);
		BufferWithArrays[buffer_pointer] = *(uint32_t*)&(CalDataClibr->PHarray[CalFCounter].b);
		buffer_pointer++;
		CheckAndStoreCalData(&StoreIndexArray, &buffer_pointer, BufferWithArrays, &SectorWithArrayStartaddr, &SectorWithArrayCounter, 512);
		BufferWithArrays[buffer_pointer] = *(uint32_t*)&(CalDataClibr->PHarray[CalFCounter].PHmin);
		buffer_pointer++;
		CheckAndStoreCalData(&StoreIndexArray, &buffer_pointer, BufferWithArrays, &SectorWithArrayStartaddr, &SectorWithArrayCounter, 512);			
	}
	
	//«апись описани€ калибровочных структур
	BufferWithStructs[Structs_descr_pointer] = *(uint32_t*)&(CalDataClibr->gZmin);
	Structs_descr_pointer++;
	CheckAndStoreCalData(&StoreIndex, &Structs_descr_pointer, BufferWithStructs, &FlashCalStructStartaddr, &SectorWithCalStructNum, 512);
	BufferWithStructs[Structs_descr_pointer] = *(uint32_t*)&(CalDataClibr->gZmax);
	Structs_descr_pointer++;
	CheckAndStoreCalData(&StoreIndex, &Structs_descr_pointer, BufferWithStructs, &FlashCalStructStartaddr, &SectorWithCalStructNum, 512);
	BufferWithStructs[Structs_descr_pointer] = *(uint32_t*)&(CalDataClibr->gPHmin);
	Structs_descr_pointer++;
	CheckAndStoreCalData(&StoreIndex, &Structs_descr_pointer, BufferWithStructs, &FlashCalStructStartaddr, &SectorWithCalStructNum, 512);
	BufferWithStructs[Structs_descr_pointer] = *(uint32_t*)&(CalDataClibr->gPHmax);
	Structs_descr_pointer++;
	CheckAndStoreCalData(&StoreIndex, &Structs_descr_pointer, BufferWithStructs, &FlashCalStructStartaddr, &SectorWithCalStructNum, 512);
	BufferWithStructs[Structs_descr_pointer] = CalDataClibr->nFmin;
	Structs_descr_pointer++;
	CheckAndStoreCalData(&StoreIndex, &Structs_descr_pointer, BufferWithStructs, &FlashCalStructStartaddr, &SectorWithCalStructNum, 512);
	BufferWithStructs[Structs_descr_pointer] = CalDataClibr->nFmax;
	Structs_descr_pointer++;
	CheckAndStoreCalData(&StoreIndex, &Structs_descr_pointer, BufferWithStructs, &FlashCalStructStartaddr, &SectorWithCalStructNum, 512);
	//«апись указателей на начало массива Zarray
	BufferWithStructs[Structs_descr_pointer] = CalDataFlashAddress;
	Structs_descr_pointer++;
	CheckAndStoreCalData(&StoreIndex, &Structs_descr_pointer, BufferWithStructs, &FlashCalStructStartaddr, &SectorWithCalStructNum, 512);
	//«апись указателей на начало массива PHarray
	BufferWithStructs[Structs_descr_pointer] = CalDataFlashAddress + (CalDataClibr->nFmax - CalDataClibr->nFmin)*sizeof(Zarray_t);
	Structs_descr_pointer++;
	CheckAndStoreCalData(&StoreIndex, &Structs_descr_pointer, BufferWithStructs, &FlashCalStructStartaddr, &SectorWithCalStructNum, 512);
	StoredZCounter++;
	
	if (stop_bit == stop_saving)	// ≈сли это последн€€ запись
	{
		for (temp = buffer_pointer; temp < 128; temp++)
		{
			BufferWithArrays[temp] = 0;
		}
		for (temp = Structs_descr_pointer; temp < 128; temp++)
		{
			BufferWithStructs[temp] = 0;
		}
		buffer_pointer = 128;
		CheckAndStoreCalData(&StoreIndexArray, &buffer_pointer, BufferWithArrays, &SectorWithArrayStartaddr, &SectorWithArrayCounter, 512);
		Structs_descr_pointer = 128;
		CheckAndStoreCalData(&StoreIndex, &Structs_descr_pointer, BufferWithStructs, &FlashCalStructStartaddr, &SectorWithCalStructNum, 512);
		for (temp = 1; temp < buffer_pointer; temp++)
		{
			BufferWithStructs[temp] = 0;
		}
		BufferWithStructs[0] = StoredZCounter;
		Structs_descr_pointer = 64;
		StoreIndex = 0;
		CheckAndStoreCalData(&StoreIndex, &Structs_descr_pointer, BufferWithStructs, &SectorWithCalStructStartaddr, &SectorWithCalStructNum, 256);
	}
	// –ассчитываем адрес следующего массива Zarray_t
	CalDataFlashAddress += ( CalDataClibr->nFmax - CalDataClibr->nFmin ) * (sizeof(Zarray_t) + sizeof(PHarray_t));
}

void LoadCalData(void)
{
	extern uint16_t nZ_cal;
	nZ_cal = *((uint16_t*)SectorWithCalStructStartaddr);
	CalData = (struct CalData_struct*)(FlashCalStructStartaddr);
}

void CheckAndStoreCalData(uint8_t* StoreIndex, uint8_t* buffer_pointer, uint32_t* BufferWithArrays, uint32_t* WSectorWithCalStructStartaddr, uint8_t* WSectorWithCalStructNum, uint16_t size)
{
	unsigned int Command[5], Result[5];
	
	if ( *buffer_pointer == (size / 4))
	{
		if ((*StoreIndex)==64)
		{
			
			(*WSectorWithCalStructNum)++;
			(*WSectorWithCalStructStartaddr) += SECTOR_SIZE;
			*StoreIndex = 0;
			// prepare sector for storing arrays
			Command[0] = 50;
			Command[1] = *WSectorWithCalStructNum;
			Command[2] = *WSectorWithCalStructNum;
			DISABLEIRQ;
			mIAPEntry(Command, Result);
			ENABLEIRQ;
			
			// earase sector
			Command[0] = 52;
			Command[1] = *WSectorWithCalStructNum;
			Command[2] = *WSectorWithCalStructNum;
			Command[3] = SystemCoreClock / 1000;
			DISABLEIRQ;
			mIAPEntry(Command, Result);
			ENABLEIRQ;
			
		}
		
		*buffer_pointer = 0;
		// prepare sector
		Command[0] = 50;
		Command[1] = *WSectorWithCalStructNum;
		Command[2] = *WSectorWithCalStructNum;
		DISABLEIRQ;
		mIAPEntry(Command, Result);
		ENABLEIRQ;
		
		// write to sector
		Command[0] = 51;
		Command[1] = (uint32_t)(*WSectorWithCalStructStartaddr+((uint32_t)(*StoreIndex))*size);
		Command[2] = (uint32_t) &(BufferWithArrays[0]);
		Command[3] = size;
		Command[4] = SystemCoreClock / 1000;
		DISABLEIRQ;
		mIAPEntry(Command, Result);
		ENABLEIRQ;
		
		(*StoreIndex)++;
	}
}

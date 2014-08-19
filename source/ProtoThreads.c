#include "ProtoThreads.h"

struct pt Power_mgmt_pt, Calibration_pt;

void InitProtothreads()
{
	PT_INIT(&Power_mgmt_pt);				// Initialization of power management 
	PT_INIT(&Calibration_pt);
}
// END OF FILE

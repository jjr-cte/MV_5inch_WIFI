#ifndef __USER_DEFINE_H__
#define __USER_DEFINE_H__

#include "ota_update.h"
#include "dhcpserver.h"

typedef enum {
	pGetProperty01_1 = 1,
    pFLASHLOADER,
	pGetProperty01_2,
	pFillMemory1,
	pFillMemory2,
	pFillMemory3,
    pConfigureMemory_2,
    pFlashEraseRegionCode1000,
    pConfigureMemoryCode,
    pGetPropertyCode,
    pWriteMemoryCode,
	pFWPACKAGE,
    UNKNOWN
} SCRIPT_ITEMS;

void nxp_script_select(int script_name);

#endif // __USER_DEFINE_H__

/* EOF */

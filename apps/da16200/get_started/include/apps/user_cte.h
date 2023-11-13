#pragma once

#include "common_def.h"

#define ENABLE_CTE_DBG

#ifdef ENABLE_CTE_DBG
#define CTE_DBG                 PRINTF
#else
#define CTE_DBG(...)            do { } while (0)
#endif

typedef enum _CTE_OTA_NXP_BOOTMODE_T {
    NXP_BOOT_NORMAL,
    NXP_BOOT_OTA
} CTE_OTA_NXP_BOOTMODE_T;

extern void CTE_OTA_NXP_INIT(void);
extern void CTE_OTA_NXP_BOOTMODE(CTE_OTA_NXP_BOOTMODE_T mode);
extern void CTE_OTA_NXP_RESET(void);


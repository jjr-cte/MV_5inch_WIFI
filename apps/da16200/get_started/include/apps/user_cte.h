#pragma once

#include "common_def.h"

#define ENABLE_CTE_DBG

#ifdef ENABLE_CTE_DBG
#define CTE_DBG                 PRINTF
#else
#define CTE_DBG(...)            do { } while (0)
#endif


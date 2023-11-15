#pragma once

#include "common_def.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

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

extern int atcmd_wifi_ctrl(int argc, char *argv[]);

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
    pFWCONFIG,
	pFWPACKAGE,
    UNKNOWN
} SCRIPT_ITEMS;

extern void nxp_script_select(int script_name);

enum {
	eCTE_Socket_None = 0,
	eCTE_Socket_MD,
	eCTE_Socket_AMD,
	eCTE_Socket_ST,
	eCTE_Socket_AST,
	eCTE_Socket_RT,
	eCTE_Socket_ART,
	eCTE_Socket_FH,
	eCTE_Socket_AFH,
	eCTE_Socket_NAK,
	eCTE_Socket_Close,
	eCTE_Socket_WattingAck,
};

extern void SetCTEStatus(UINT8 status);
extern void SetCTESocketConnection(void);
extern void SetCTESocketDisconnection(void);
extern void CTESendDataToSocketServer(INT8 *data);
extern void CTEGetSocketServerData(UINT8 *payload, INT32 payload_len);
extern void CTESocketCtrlFlow(void);

extern UINT nxpcmd_parser(char *in_buf);

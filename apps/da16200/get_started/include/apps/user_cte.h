#pragma once

#include "common_def.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define ENABLE_CTE_DBG
#define CTE_DBG_DUMP_PARTIAL
#define CTE_DBG_TIME

#ifndef ENABLE_CTE_DBG
#undef CTE_DBG_DUMP_PARTIAL
#undef CTE_DBG_TIME
#endif

#ifdef ENABLE_CTE_DBG
#define CTE_DBG                 PRINTF
#define memory_dump(p, l)       hex_dump((UCHAR *) p, (UINT) l)
#else
#define CTE_DBG(...)            do { } while (0)
#define memory_dump(p, l)       do { } while (0)
#endif

#ifdef CTE_DBG_TIME
#define CTE_DBG_PRINT_TS()      CTE_DBG(GREEN_COLOR "[T=%5ld] " CLEAR_COLOR, xTaskGetTickCount())
#else
#define CTE_DBG_PRINT_TS()      do { } while (0)
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

#define NXP_OTA_UART_WRITE(x)   NXP_OTA_UART_WRITE_N(x, sizeof(x))
extern void NXP_OTA_UART_WRITE_N(const uint8_t *x, size_t n);

#define NXP_OTA_MSG_APPEND(payload, idx, data)              \
    do {                                                    \
        payload[idx++] = (uint8_t) ( data        & 0xFF);   \
        payload[idx++] = (uint8_t) ((data >>  8) & 0xFF);   \
        payload[idx++] = (uint8_t) ((data >> 16) & 0xFF);   \
        payload[idx++] = (uint8_t) ((data >> 24) & 0xFF);   \
    } while (0)

extern void SetCTEStatus(UINT8 status);
extern void SetCTESocketConnection(void);
extern void SetCTESocketDisconnection(void);
extern void CTESendDataToSocketServer(INT8 *data);
extern void CTEGetSocketServerData(UINT8 *payload, INT32 payload_len);
extern void CTESocketCtrlFlow(void);

extern void generate_crc_table(void);

extern UINT nxpcmd_parser(char *in_buf);

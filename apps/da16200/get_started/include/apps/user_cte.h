#pragma once

#include "common_def.h"

#include "mbedtls/md5.h"

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
#define md5dump(x)              memory_dump(x, 16)
#define CTE_DBG_XXX()           CTE_DBG(RED_COLOR "XXX\n" CLEAR_COLOR)
#else
#define CTE_DBG(...)            do { } while (0)
#define memory_dump(p, l)       do { } while (0)
#define md5dump(x)              do { } while (0)
#define CTE_DBG_XXX(...)        do { } while (0)
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

extern SemaphoreHandle_t    CTE_OTA_NXP_READY_mutex;
extern uint32_t             CTE_OTA_NXP_READY;

#define ATCMD_OTAOK_CMD     "OTAOK"

extern void CTE_OTA_NXP_INIT(void);
extern void CTE_OTA_NXP_BOOTMODE(CTE_OTA_NXP_BOOTMODE_T mode);
extern void CTE_OTA_NXP_RESET(void);

extern TaskHandle_t         CTE_OTA_TaskHandler;

extern int atcmd_otaok(int argc, char *argv[]);
extern int atcmd_wifi_ctrl(int argc, char *argv[]);

#define SCRIPT_ENUM                         \
    ENUM_ENTRY(pNone),                      \
    ENUM_ENTRY(pGetProperty01_1),           \
    ENUM_ENTRY(pFLASHLOADER),               \
    ENUM_ENTRY(pGetProperty01_2),           \
    ENUM_ENTRY(pFillMemory1),               \
    ENUM_ENTRY(pFillMemory2),               \
    ENUM_ENTRY(pFillMemory3),               \
    ENUM_ENTRY(pConfigureMemory_2),         \
    ENUM_ENTRY(pFlashEraseRegionCode1000),  \
    ENUM_ENTRY(pConfigureMemoryCode),       \
    ENUM_ENTRY(pGetPropertyCode),           \
    ENUM_ENTRY(pWriteMemoryCode),           \
    ENUM_ENTRY(pFWCONFIG),                  \
    ENUM_ENTRY(pFWPACKAGE),                 \
    ENUM_ENTRY(UNKNOWN)

#define ENUM_ENTRY(x)   x

typedef enum _cte_ota_script_t {
    SCRIPT_ENUM,
    NUM_SCRIPTS
} cte_ota_script_t;

#undef ENUM_ENTRY

extern const char *cte_ota_script_str[];

extern void nxp_script_select(cte_ota_script_t script_name);

#define STATUS_ENUM         \
    ENUM_ENTRY(None),       \
    ENUM_ENTRY(MD),         \
    ENUM_ENTRY(AMD),        \
    ENUM_ENTRY(ST),         \
    ENUM_ENTRY(AST),        \
    ENUM_ENTRY(RT),         \
    ENUM_ENTRY(ART),        \
    ENUM_ENTRY(RST),        \
    ENUM_ENTRY(ARST),       \
    ENUM_ENTRY(FH),         \
    ENUM_ENTRY(AFH),        \
    ENUM_ENTRY(NAK),        \
    ENUM_ENTRY(Close),      \
    ENUM_ENTRY(WaitingAck)

#define ENUM_ENTRY(x)   eCTE_Socket_ ## x

typedef enum _cte_status_t {
    STATUS_ENUM,
    NUM_STATUS
} cte_status_t;

#undef ENUM_ENTRY

extern const char *cte_status_str[];

#define NXP_OTA_UART_WRITE(x)   NXP_OTA_UART_WRITE_N(x, sizeof(x))
extern void NXP_OTA_UART_WRITE_N(const uint8_t *x, size_t n);

#define NXP_OTA_MSG_APPEND(payload, idx, data)              \
    do {                                                    \
        payload[idx++] = (uint8_t) ( data        & 0xFF);   \
        payload[idx++] = (uint8_t) ((data >>  8) & 0xFF);   \
        payload[idx++] = (uint8_t) ((data >> 16) & 0xFF);   \
        payload[idx++] = (uint8_t) ((data >> 24) & 0xFF);   \
    } while (0)

#define CTE_OTA_RETX_MAX        3
#define CTE_OTA_TIMEOUT_ST      30      // 3000 ms
#define CTE_OTA_TIMEOUT_RT      5       // 500 ms
#define CTE_OTA_TIMEOUT_RST     5       // 500 ms

#define PTYPE_ENUM      \
    ENUM_ENTRY(MD),     \
    ENUM_ENTRY(AMD),    \
    ENUM_ENTRY(ST),     \
    ENUM_ENTRY(AST),    \
    ENUM_ENTRY(RT),     \
    ENUM_ENTRY(ART),    \
    ENUM_ENTRY(RST),    \
    ENUM_ENTRY(ARST),   \
    ENUM_ENTRY(FH),     \
    ENUM_ENTRY(AFH)

#define ENUM_ENTRY(x)   OTA_PKT_TYPE_ ## x

typedef enum _ota_packet_type_t {
    PTYPE_ENUM,
    NUM_PTYPE
} ota_packet_type_t;

#undef ENUM_ENTRY

extern const char *ota_packet_type_str[];

/*
 * OTA_PKT_TYPE_MD
 * OTA_PKT_TYPE_ST
 * OTA_PKT_TYPE_AST
 * OTA_PKT_TYPE_RST
 * OTA_PKT_TYPE_ARST
 * OTA_PKT_TYPE_FH
 */
typedef struct _ota_packet_t {
    uint8_t     type;
} ota_packet_t;

typedef ota_packet_t        ota_packet_md_t;
typedef ota_packet_t        ota_packet_st_t;
typedef ota_packet_t        ota_packet_ast_t;
typedef ota_packet_t        ota_packet_rst_t;
typedef ota_packet_t        ota_packet_arst_t;
typedef ota_packet_t        ota_packet_fh_t;

/*
 * OTA_PKT_TYPE_AMD
 */
typedef struct _ota_packet_amd_t {
    uint8_t     type;
    uint8_t     _pad[3];
    uint32_t    total_size;
    uint32_t    total_packet;
} ota_packet_amd_t;

/*
 * OTA_PKT_TYPE_RT
 */
typedef struct _ota_packet_rt_t {
    uint8_t     type;
    uint8_t     _pad[3];
    uint32_t    packet_number;
} ota_packet_rt_t;

/*
 * OTA_PKT_TYPE_ART
 */
typedef struct _ota_packet_art_t {
    uint8_t     type;
    uint8_t     checksum;
    uint8_t     _pad[2];
    uint32_t    packet_number;
    uint32_t    packet_size;
    uint8_t     payload[512];
} ota_packet_art_t;

/*
 * OTA_PKT_TYPE_AFH
 */
typedef struct _ota_packet_afh_t {
    uint8_t     type;
    uint8_t     _pad[3];
    uint8_t     md5sum[16];
} ota_packet_afh_t;

//Timmy
extern int g_ota_mcu_uart_idx;

typedef struct _cte_ota_ctx_t {
    uint32_t    total_size;
    uint32_t    total_packet;
    uint32_t    cur_packet;
    uint16_t    cur_size;
    uint8_t     retx_cnt;
    QueueHandle_t           q_p;
    mbedtls_md5_context     md5_ctx;
} cte_ota_ctx_t;

extern cte_ota_ctx_t* cte_ota_get_context(void);

extern void CTEStatusSet(const cte_status_t status);
extern cte_status_t CTEStatusGet(void);
extern void SetCTESocketConnection(void);
extern void SetCTESocketDisconnection(void);
extern void CTESendDataToSocketServer(const void *data, const size_t data_len);
extern void CTEGetSocketServerData(cte_ota_ctx_t *ctx, UINT8 *payload);
extern int CTESocketCtrlFlow(cte_ota_ctx_t *ctx);

extern void generate_crc_table(void);

extern UINT nxpcmd_parser(char *in_buf);

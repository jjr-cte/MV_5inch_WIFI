#pragma once

#include "common_def.h"
#include "atcmd.h"
#include "mbedtls/md5.h"


#define CTE_OTA_SERVER_PORT 		2300
#define CTE_OTA_RETX_MAX        	99999
#define CTE_FLASH_WRITE_RETX_MAX	5
#define CTE_OTA_TIMEOUT         	pdMS_TO_TICKS(6000)

#define CTEVER_P    "P03"	//must be 3 characters
#define CTEVER_T    "T00"	//must be 3 characters

#define CTE_DBG_DUMP_PARTIAL
#define CTE_DBG_TIME


#ifndef RELEASE_BUILD
#define ENABLE_CTE_DBG
#endif

#ifndef ENABLE_CTE_DBG
#undef CTE_DBG_DUMP_PARTIAL
#undef CTE_DBG_TIME
#endif

#ifdef ENABLE_CTE_DBG
#define CTE_DBG                 PRINTF
#define memory_dump(p, l)       CTE_hex_dump(p, l)
#define md5dump(x)              memory_dump(x, 16)
#define CTE_DBG_XXX()           CTE_DBG(RED_COLOR "XXX\n" CLEAR_COLOR)
#define CTE_DBG_FATAL(...)		\
do { \
	CTE_DBG(RED_COLOR); \
	CTE_DBG(__VA_ARGS__); \
	CTE_DBG(CLEAR_COLOR); \
} while(0)
#else
#define CTE_DBG(...)            do { } while (0)
#define memory_dump(p, l)       do { } while (0)
#define md5dump(x)              do { } while (0)
#define CTE_DBG_XXX()           do { } while (0)
#define CTE_DBG_FATAL(...)		do { } while (0)
#endif

#ifdef CTE_DBG_TIME
#define CTE_DBG_PRINT_TS()      CTE_DBG(GREEN_COLOR "[T=%5ld] " CLEAR_COLOR, xTaskGetTickCount())
#else
#define CTE_DBG_PRINT_TS()      do { } while (0)
#endif

#define NXP_MCUBOOT_STARTBYTE 0x5A


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
 *      app         wifi            NXP
 *       | <-     -> | <-         -> |
 *            ^              ^
 *     OTA_CTE_APP_LEN  OTA_CTE_NXP_LEN
 */
#define OTA_CTE_NXP_LEN             1024
#define OTA_CTE_NXP_PKTS            8
#define OTA_CTE_APP_LEN             (OTA_CTE_NXP_LEN * OTA_CTE_NXP_PKTS)

/* confirmed with app provider */
#if OTA_CTE_APP_LEN != 8192
    #error
#endif

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
    uint8_t     ota_type;
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
    uint8_t     ota_type;
    uint8_t     _pad[2];
    uint32_t    total_size;
    uint32_t    total_packet;
} ota_packet_amd_t;

/*
 * OTA_PKT_TYPE_RT
 */
typedef struct _ota_packet_rt_t {
    uint8_t     type;
    uint8_t     ota_type;
    uint8_t     _pad[2];
    uint32_t    packet_number;
} ota_packet_rt_t;

/*
 * OTA_PKT_TYPE_ART
 */
typedef struct _ota_packet_art_t {
    uint8_t     type;
    uint8_t     ota_type;
    uint8_t     checksum;
    uint8_t     _pad[1];
    uint32_t    packet_number;
    uint32_t    packet_size;
    uint8_t     payload[OTA_CTE_APP_LEN];
} ota_packet_art_t;

/*
 * OTA_PKT_TYPE_AFH
 */
typedef struct _ota_packet_afh_t {
    uint8_t     type;
    uint8_t     ota_type;
    uint8_t     _pad[2];
    uint8_t     md5sum[16];
} ota_packet_afh_t;

/*
 * OTA_PKT_TYPE_RV
 */
typedef struct {
    uint8_t     type;
    uint8_t     ota_type;
    char ver[6];

} ota_packet_rv_t;



#ifdef ENABLE_CTE_DBG
extern void CTE_hex_dump(const void *, size_t);
#endif


extern void CTE_OTA_GPIO_INIT(void);

typedef enum _CTE_OTA_NXP_BOOTMODE_T {
    NXP_BOOT_NORMAL,
    NXP_BOOT_OTA
} CTE_OTA_NXP_BOOTMODE_T;

extern void CTE_OTA_NXP_BOOTMODE(CTE_OTA_NXP_BOOTMODE_T mode);

extern void CTE_OTA_NXP_RESET(void);

extern bool CTE_OTA_NXP_ALIVE();


extern int atcmd_wifi_ctrl(int argc, char *argv[]);
extern int atcmd_ctever(int argc, char *argv[]);
extern int atcmd_nxp_conn(int argc, char *argv[]);
extern int atcmd_ota_type(int argc, char *argv[]);


typedef enum _ota_type_t {
	OTA_TYPE_NXP,
	OTA_TYPE_WIFI,
	OTA_TYPE_NULL = 0xFF
} ota_type_t;

extern void ota_type_set(ota_type_t t);
extern ota_type_t ota_type_get(void);
extern void ota_type_invalidate(void);


extern void nxp_mcuboot_send_ack();
extern void nxp_mcuboot_send_data_packet(const void *data, size_t length);
extern void nxp_mcuboot_parser(uint8_t ch);
extern bool is_dashboard_fw_recovery_mode();
extern void dashboard_recovery_mode_state_machine();

extern void monitor_dashboard_alive();
extern void UserApp_DRM_Monitor(void *);

extern void nxpcmd_parser(const uint8_t *in_buf);


// CTE OTA Server task function
extern void UserApp_CTE_OTA(void *);

// notify CTE OTA Server task that WiFi-STA(user's mobile) is disconnected
extern void CTE_OTA_WiFi_disassociated();

// notify CTE OTA Server task that ERASE command to dashboard is done
// erase_ok: *true* if received "AT+ERASEOK", *false* if received "AT+ERASENG"
extern void CTE_OTA_Nxp_erase_answer(bool erase_ok);

// notify CTE OTA Server task that WRITE_BLOCK command to dashboard is OK
extern void CTE_OTA_Nxp_write_block_ok();

// notify CTE OTA Server task that retry WRITE_BLOCK command to dashboard
extern void CTE_OTA_Nxp_write_block_rewrite();

// notify CTE OTA Server task that stop sending WRITE_BLOCK command to dashboard
extern void CTE_OTA_Nxp_write_block_abort();

// notify CTE OTA Server task that dashboard firmware write is finished
extern void CTE_OTA_Nxp_flash_finished();

// notify CTE OTA Server task of dashboard firmware MD5 check result
// md5_ok: true* if received "AT+MD5OK", *false* if received "AT+MD5NG"
extern void CTE_OTA_MD5_result(bool md5_ok);

// enter dashboard recovery mode
extern void CTE_Enter_Dashboard_Recovery_Mode();

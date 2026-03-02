#pragma once

#include "common_def.h"
#include "atcmd.h"
#include "mbedtls/md5.h"

#define CTEVER_P    "P06"	//must be 3 characters
#define CTEVER_T    "T01"	//must be 3 characters


#define CTE_OTA_SERVER_PORT 			2300		//OTA Server 使用的port
#define CTE_OTA_KEEPALIVE_INTERVAL 		4000		//KeepAlive發送間隔(unit: ms)
#define CTE_OTA_MCU_STUCK_TIMEOUT		20000		//MCU卡死的超時長度(unit: ms)
#define CTE_OTA_MD5_CALC_WAIT_TIME		1300000		//等待MCU計算映像檔MD5的時間(unit: ms)//JJR
#define CTE_OTA_MCU_STUCK_REBOOT_DELAY	6000		//判定MCU卡死後，延遲多久發RST訊號(unit: ms)
#define CTE_OTA_MAIN_FW_SIZE_LIMIT		0x1E00000	//主程式尺寸限制(unit: bytes)
#define CTE_OTA_WIFI_FW_SIZE_LIMIT		0x1A0000	//WIFI韌體尺寸限制(1664KB)
#define CTE_OTA_BT_FW_SIZE_LIMIT        0x200000    //BT dummy limit = 2MB (TBD)
#define CTE_OTA_BLE_FW_SIZE_LIMIT       0x200000    //BLE dummy limit = 2MB (TBD)
#define CTE_OTA_TCP_ACCEPT_COOLDOWN		300			//OTA Server accept客戶端的間隔(unit: ms)
#define CTE_OTA_WIFI_DATA_TIMEOUT		35000		//超過多久沒收到WIFI韌體就判為錯誤(unit: ms)
#define CTE_OTA_RESTART_STATE_MACHINE_DELAY		12000	//更新成功後多久重啟OTA狀態機(unit: ms)
#define CTE_OTA_RESTART_WIFI_DELAY      3000        //更新成功後多久重啟WIFI(unit: ms)
#define CTE_OTA_RESTART_BT_DELAY      5000        //更新成功後多久重啟BT(unit: ms)
#define CTE_OTA_RESTART_BLE_DELAY      5000        //更新成功後多久重啟BLE(unit: ms)
#define CTE_OTA_UNKNOW_DELAY            3000        //unkonw reboot

#define CTE_RECOVER_INIT_WAIT_TIME		2500	// 更新後等待MCU重開的時間
#define CTE_RECOVER_POLLING_INTERVAL	300		// alive polling間隔
#define CTE_RECOVER_TOTAL_POLLING_AFTER_UPDATE	5	// alive polling次數 (更新後)
#define CTE_RECOVER_TOTAL_POLLING_ON_STARTUP	4	// alive polling次數 (開機時)
#define CTE_RECOVER_ENV_NAME			"CHECK_DSB"
#define CTE_RECOVER_ACTIVE_VALUE		"CTE"


#ifndef RELEASE_BUILD
#define ENABLE_CTE_DBG
#endif

#define CTE_DBG_FATAL(...)		\
do { \
	PRINTF(RED_COLOR); \
	PRINTF(__VA_ARGS__); \
	PRINTF(CLEAR_COLOR); \
} while(0)

#ifdef ENABLE_CTE_DBG
#define CTE_DBG                 PRINTF
#define memory_dump(p, l)       CTE_hex_dump(p, l)
#define md5dump(x)              memory_dump(x, 16)
#define CTE_DBG_XXX()           CTE_DBG(RED_COLOR "XXX " CLEAR_COLOR)
#define CTE_DBG_PRINT_TS()      CTE_DBG(GREEN_COLOR "[%4lu] " CLEAR_COLOR, xTaskGetTickCount())
#else
#define CTE_DBG(...)            do { } while (0)
#define memory_dump(p, l)       do { } while (0)
#define md5dump(x)              do { } while (0)
#define CTE_DBG_XXX()           do { } while (0)
#define CTE_DBG_PRINT_TS()      do { } while (0)
#endif


#define NXP_MCUBOOT_STARTBYTE 0x5A



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


#ifdef ENABLE_CTE_DBG
extern void CTE_hex_dump(const void *, size_t);
#endif


extern void CTE_GPIO_INIT(void);

typedef enum _CTE_OTA_NXP_BOOTMODE_T {
    NXP_BOOT_NORMAL,
    NXP_BOOT_SERIAL_DOWNLOADER,
} CTE_OTA_NXP_BOOTMODE_T;

extern void CTE_NXP_BOOTMODE(CTE_OTA_NXP_BOOTMODE_T mode);

extern void CTE_NXP_RESET(void);

extern bool CTE_NXP_ALIVE(void);


extern int atcmd_wifi_ctrl(int argc, char *argv[]);
extern int atcmd_ctever(int argc, char *argv[]);
extern int atcmd_nxp_conn(int argc, char *argv[]);
extern int atcmd_ota_type(int argc, char *argv[]);


extern void nxp_mcuboot_send_ack(void);
extern void nxp_mcuboot_send_data_packet(const void *data, size_t length);
extern void nxp_mcuboot_parser(uint8_t ch);
extern bool is_dashboard_fw_recovery_mode(void);
extern void dashboard_recovery_mode_state_machine(void);

extern void monitor_dashboard_alive(void);
extern void UserApp_DRM_Monitor(void *);

extern void nxpcmd_parser(const uint8_t *in_buf);

extern bool CTE_RECOVER_need_check_dashboard_alive(void);
extern void CTE_RECOVER_set_flag(void);
extern void CTE_RECOVER_clear_flag(void);

extern void CTE_AP_control(bool on_off);
extern void CTE_WFSAP_change_AP_config(void);
extern void UserApp_AP_Controller(void *);


// CTE OTA Server task function
extern void UserApp_CTE_OTA(void *);

// notify CTE OTA Server task that ERASE command to MCU is done
// erase_ok: *true* if received "AT+ERASEOK", *false* if received "AT+ERASENG"
extern void CTE_OTA_Nxp_erase_result(bool erase_ok);

// notify CTE OTA Server task that WRITE_BLOCK command to MCU is OK
extern void CTE_OTA_Nxp_write_block_ok(void);

// notify CTE OTA Server task that stop sending WRITE_BLOCK command to MCU
// err_code: the error code reported by MCU
extern void CTE_OTA_Nxp_write_block_abort(uint8_t err_code);

// notify CTE OTA Server task that MCU firmware write is finished
extern void CTE_OTA_Nxp_flash_finished(void);

// notify CTE OTA Server task of MCU firmware MD5 check result
// md5_ok: true* if received "AT+MD5OK", *false* if received "AT+MD5NG"
extern void CTE_OTA_Nxp_MD5_check_result(bool md5_ok);

// enter dashboard recovery mode
extern void CTE_Enter_Dashboard_Recovery_Mode(void);

// restart OTA automata
extern void CTE_OTA_restart(void);

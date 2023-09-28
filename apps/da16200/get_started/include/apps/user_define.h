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

ota_update_type ota_update_http_client_get_run_fw_type(void);
ip4_addr_t* dhcpd_get_remote_address(void);
void myCommand(char *messages);
int create_tcpc_manual(void);
void cancel_all_socket_session(int cid);
void sendInformationToMessageServer(char *messages, int message_length);
void nxp_script_select(int script_name);

#endif // __USER_DEFINE_H__

/* EOF */

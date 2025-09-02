#include "sdk_type.h"
#include "user_main.h"
#include "common_def.h"
#include "common_uart.h"
#include "util_api.h"
#include "sys_specific.h"
#include "atcmd.h"
#include "user_cte.h"
#include "nvedit.h"
#include "environ.h"



#ifdef ENABLE_CTE_DBG
void CTE_hex_dump(const void *p, size_t len)
{
	if (len == 0) return;

	//__[001A] 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
	// total 53 char. allocate 64bytes space.
	const size_t BUFFER_SIZE = 64;
	char *buf = pvPortMalloc(BUFFER_SIZE);

	if (buf == NULL)
	{
		configASSERT(0);
		return;
	}

	char *itr = buf;
	size_t remain = BUFFER_SIZE;

	uint8_t *data = (uint8_t *) p;
	unsigned lno = 1;
	int sz;

	for (size_t i = 0; i < len; i++)
	{
		// beginning of line
		if ((i & 15) == 0)
		{
			if ((sz = snprintf(itr, remain, "  [%3u]", lno)) > 0)
			{
				itr += sz;
				remain -= sz;
			}
			lno++;
		}

		if ((sz = snprintf(itr, remain, " %02X", *data)) > 0)
		{
			itr += sz;
			remain -= sz;
		}
		data++;

		if ((i & 15) == 15)
		{
			PRINTF("%s\n", buf);
			itr = buf;
			remain = BUFFER_SIZE;
			*buf = '\0';
		}
	}

	if (*buf != '\0')
	{
		PRINTF("%s\n", buf);
	}

	vPortFree(buf);
}
#endif


void CTE_GPIO_INIT(void)
{
	HANDLE gpioA = GPIO_CREATE(GPIO_UNIT_A);

	GPIO_INIT(gpioA);

	//A2: initial high
	UINT16 pin_value = GPIO_PIN2;
	GPIO_WRITE(gpioA, GPIO_PIN2, &pin_value, sizeof(pin_value));

	//A3: initial low
	pin_value = 0;
	GPIO_WRITE(gpioA, GPIO_PIN3, &pin_value, sizeof(pin_value));

	//A2, A3: output
	UINT32 pin_numbers = GPIO_PIN2 | GPIO_PIN3;
	GPIO_IOCTL(gpioA, GPIO_SET_OUTPUT, &pin_numbers);

	//A10, A11: input
	//A10: MCU Alive訊號線
	pin_numbers = GPIO_PIN10 | GPIO_PIN11;
	GPIO_IOCTL(gpioA, GPIO_SET_INPUT, &pin_numbers);
}


void CTE_NXP_BOOTMODE(const CTE_OTA_NXP_BOOTMODE_T mode)
{
    CTE_DBG("%s(), mode = %d\n", __func__, mode);

    HANDLE gpioA = GPIO_CREATE(GPIO_UNIT_A);
    configASSERT(gpioA);
    if (gpioA == NULL) return;

    UINT16 pin_value;

    switch (mode)
    {
    case NXP_BOOT_NORMAL:
    	pin_value = 0;	//low
    	break;
    case NXP_BOOT_SERIAL_DOWNLOADER:
    	pin_value = GPIO_PIN3;	//high
        break;
    default :
        configASSERT(0);
        return;
    }

	GPIO_WRITE(gpioA, GPIO_PIN3, &pin_value, sizeof(pin_value));
}


void CTE_NXP_RESET(void)
{
    CTE_DBG("%s()\n", __func__);

    HANDLE gpioA = GPIO_CREATE(GPIO_UNIT_A);
    configASSERT(gpioA);
    if (gpioA == NULL) return;

    UINT16 high = GPIO_PIN2;
    UINT16 low = 0;

    GPIO_WRITE(gpioA, GPIO_PIN2, &high, sizeof(UINT16));
    vTaskDelay(pdMS_TO_TICKS(100));
    GPIO_WRITE(gpioA, GPIO_PIN2, &low, sizeof(UINT16));
    vTaskDelay(pdMS_TO_TICKS(100));
    GPIO_WRITE(gpioA, GPIO_PIN2, &high, sizeof(UINT16));
    vTaskDelay(pdMS_TO_TICKS(100));
}


bool CTE_NXP_ALIVE(void)
{
	HANDLE gpio = GPIO_CREATE(GPIO_UNIT_A);
	UINT32 pin = GPIO_PIN10;
	UINT16 pv;
	GPIO_READ(gpio, pin, &pv, sizeof(pv));
	return pv != 0;
}


////////////////////////////////////////////////////////////////////////////////


int atcmd_wifi_ctrl(int argc, char *argv[])
{
    int err_code = AT_CMD_ERR_CMD_OK;

    CTE_DBG("%s()\n", __func__);

    if (argc != 2)
    {
        err_code = AT_CMD_ERR_UNKNOWN;
    }
    else
    {
        CTE_DBG("argv[1] = %s\n", argv[1]);

        if (0 == strcasecmp(argv[1], "on"))
        {
        	// dust.240719: use CLI commands to turn on WIFI, instead control RF power
        	CTE_AP_control(true);
        }
        else if (0 == strcasecmp(argv[1], "off"))
        {
        	// dust.240719: use CLI commands to turn off WIFI, instead control RF power
        	CTE_AP_control(false);
        	CTE_OTA_restart();
        }
        else
        {
            err_code = AT_CMD_ERR_UNKNOWN;
        }
    }

    return err_code;
}


int atcmd_ctever(int argc, char *argv[])
{
    int     err_code = AT_CMD_ERR_CMD_OK_WO_PRINT;

    PRINTF("%s(): %s%s\n", __func__, CTEVER_P, CTEVER_T);

    if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
        PRINTF_ATCMD("\r\n+CTEVER:%s%s\r\n", CTEVER_P, CTEVER_T);
    }
    else {
        err_code = AT_CMD_ERR_TOO_MANY_ARGS;
    }

    return err_code;
}


int atcmd_nxp_conn(int argc, char *argv[])
{
    int err_code = AT_CMD_ERR_CMD_OK_WO_PRINT;

    DA16X_UNUSED_ARG(argc);

    CTE_DBG("%s(), %s\n", __func__, argv[0]);

    if (0 == strcasecmp(argv[0] + 3, "ERASEOK"))
    {
    	CTE_OTA_Nxp_erase_result(true);
    }
    else if (0 == strcasecmp(argv[0] + 3, "ERASENG"))
    {
    	CTE_OTA_Nxp_erase_result(false);
    }
    else if (0 == strcasecmp(argv[0] + 3, "MD5OK"))
    {
    	CTE_OTA_Nxp_MD5_check_result(true);
    }
    else if (0 == strcasecmp(argv[0] + 3, "MD5NG"))
    {
    	CTE_OTA_Nxp_MD5_check_result(false);
    }
    else
    {
        err_code = AT_CMD_ERR_UNKNOWN_CMD;
    }

    return err_code;
}


int atcmd_ota_type(int argc, char *argv[])
{
    int err_code = AT_CMD_ERR_CMD_OK;

    CTE_DBG("%s()\n", __func__);

    if (argc != 2)
    {
    	CTE_DBG("  invalid argc = %d\n", argc);
        err_code = AT_CMD_ERR_UNKNOWN;
    }
    else
    {
        CTE_DBG("  argv[1] = %s\n", argv[1]);

        if (0 == strcasecmp(argv[1], "nxp"))
        {
#if 0	//dust.240202: didn't use pre-configured OTA-type anymore
        	ota_type_set(OTA_TYPE_NXP);
#endif
        }
        else if (0 == strcasecmp(argv[1], "wifi"))
        {
#if 0	//dust.240202: didn't use pre-configured OTA-type anymore
            ota_type_set(OTA_TYPE_WIFI);
#endif
        }
        else
        {
            err_code = AT_CMD_ERR_UNKNOWN;
        }
    }

    return err_code;
}


////////////////////////////////////////////////////////////////////////////////

/*
 * NXP OTA flow :
 *
 * host     NXP     message             meaning
 *      -->         5A A6               ping
 *      <--         5A A7               ping response
 *
 *      -->         5A A4               command pGetProperty01_1
 *      <--         5A A1 5A A4         command ack
 *      -->         5A A1               ack
 *
 *      -->         5A A5               data payload : pFLASHLOADER
 *      <--         5A A1               ack
 *      -->         5A A5               data payload : pFLASHLOADER (continue)
 *      <--         5A A1               ack
 *                   ...
 *
 *      -->         5A A6               ping
 *      <--         5A A7               ping response
 *
 *      -->         5A A4               command pGetProperty01_2
 *      <--         5A A1 5A A4         command ack
 *      -->         5A A1               ack
 *
 *      -->         5A A4               command pFillMemory1
 *      <--         5A A1 5A A4         command ack
 *      -->         5A A1               ack
 *
 *      -->         5A A4               command pFillMemory2
 *      <--         5A A1 5A A4         command ack
 *      -->         5A A1               ack
 *
 *      -->         5A A4               command pFillMemory3
 *      <--         5A A1 5A A4         command ack
 *      -->         5A A1               ack
 *
 *      -->         5A A4               command pConfigureMemory_2
 *      <--         5A A1 5A A4         command ack
 *      -->         5A A1               ack
 *
 *      -->         5A A4               command pFlashEraseRegionCode1000
 *      <--         5A A1 5A A4         command ack
 *      -->         5A A1               ack
 *
 *      -->         5A A4               command pConfigureMemoryCode
 *      <--         5A A1 5A A4         command ack
 *      -->         5A A1               ack
 *
 *      -->         5A A4               command pGetPropertyCode
 *      <--         5A A1 5A A4         command ack
 *      -->         5A A1               ack
 *
 *      -->         5A A4               command pWriteMemoryCode
 *      <--         5A A1 5A A4         command ack
 *      -->         5A A1               ack
 *
 *      -->         5A A5               data payload : pFWCONFIG
 *      <--         5A A1               ack
 *      -->         5A A5               data payload : pFWCONFIG (continue)
 *      <--         5A A1               ack
 *                   ...
 *
 *      -->         5A A5               data payload : pFWPACKAGE
 *      <--         5A A1               ack
 *      -->         5A A5               data payload : pFWPACKAGE (continue)
 *      <--         5A A1               ack
 *                   ...
 *      <--         5A A1 5A A4         command ack : write memory finished
 *      -->         5A A1               ack
 */
/*
 * under AT-CMD task, priority OS_TASK_PRIORITY_USER + 6
 * call path : atcmd_task() -> monitor_atcmd() -> nxpcmd_parser()
 */
void nxpcmd_parser(const uint8_t *in_buf)
{
	switch (in_buf[1])
	{
	case 0xA1:
		CTE_OTA_Nxp_write_block_ok();
		break;

	case 0xA2:
		CTE_DBG_FATAL(">> 5A A2\n");
		CTE_OTA_Nxp_write_block_abort(0xA2);
		break;

	case 0xA3:
		CTE_DBG_FATAL(">> 5A A3\n");
		CTE_OTA_Nxp_write_block_abort(0xA3);
		break;

	case 0xA4:
		CTE_OTA_Nxp_flash_finished();
		break;

	case 0xFE:
		CTE_DBG_FATAL(">> 5A FE\n");
		CTE_OTA_Nxp_write_block_abort(0xFE);
		break;

	case 0xFF:
		CTE_DBG_FATAL(">> 5A FF\n");
		CTE_OTA_Nxp_write_block_abort(0xFF);
		break;

	default:
		CTE_DBG_FATAL(">> 5A '%02X'\n", in_buf[1]);
		break;
	}
}


////////////////////////////////////////////////////////////////////////////////


bool CTE_RECOVER_need_check_dashboard_alive(void)
{
	char *value;
	value = da16x_getenv(ENVIRON_DEVICE, "app", CTE_RECOVER_ENV_NAME);

	if (value == NULL)
	{
		return false;
	}

	return 0 == strcmp(value, CTE_RECOVER_ACTIVE_VALUE);
}


void CTE_RECOVER_set_flag(void)
{
	da16x_setenv(ENVIRON_DEVICE, "app", CTE_RECOVER_ENV_NAME, CTE_RECOVER_ACTIVE_VALUE);
}


void CTE_RECOVER_clear_flag(void)
{
	da16x_unsetenv(ENVIRON_DEVICE, "app", CTE_RECOVER_ENV_NAME);
}


////////////////////////////////////////////////////////////////////////////////



static SemaphoreHandle_t sem_drmm = NULL;

static void init_sem_drmm(void)
{
	if (sem_drmm != NULL) return;

	static StaticSemaphore_t instance;
	sem_drmm = xSemaphoreCreateBinaryStatic(&instance);
}

void monitor_dashboard_alive(void)
{
	CTE_DBG_PRINT_TS();
	CTE_DBG("%s()\n", __func__);

	init_sem_drmm();
	xSemaphoreGive(sem_drmm);
}

void UserApp_DRM_Monitor(void *arg)
{
	DA16X_UNUSED_ARG(arg);

	init_sem_drmm();

	while (1)
	{
		if (!xSemaphoreTake(sem_drmm, portMAX_DELAY)) continue;

		CTE_DBG_PRINT_TS();
		CTE_DBG("DRMM: waiting\n");
		vTaskDelay(pdMS_TO_TICKS(CTE_RECOVER_INIT_WAIT_TIME));

		bool dashboard_alive = false;
		for (int i = 0; i < CTE_RECOVER_TOTAL_POLLING_AFTER_UPDATE; i++)
		{
			vTaskDelay(pdMS_TO_TICKS(CTE_RECOVER_POLLING_INTERVAL));

			if (CTE_NXP_ALIVE())
			{
				dashboard_alive = true;
				CTE_DBG_PRINT_TS();
				CTE_DBG("DRMM: detect alive\n");
				break;
			}
			else
			{
				CTE_DBG_PRINT_TS();
				CTE_DBG("DRMM: detecting...\n");
			}
		}

		if (!dashboard_alive)
		{
			PRINTF("activate Dashboard Recovery Mode from DRMM\n");
			CTE_Enter_Dashboard_Recovery_Mode();
		}
		else
		{
			PRINTF("remove RECOVER flag by DRMM\n");
			CTE_RECOVER_clear_flag();
		}
	}
}


////////////////////////////////////////////////////////////////////////////////


#define AP_ON	1
#define AP_OFF	2
#define WFSAP_CHANGE_CONFIG	3


static QueueHandle_t command_queue_APCtrl = NULL;



void CTE_AP_control(bool on_off)
{
	if (command_queue_APCtrl == NULL) return;

	uint8_t cmd;

	if (on_off)
	{
		PRINTF("start AP asynchronously\n");
		cmd = AP_ON;
	}
	else
	{
		PRINTF("stop AP asynchronously\n");
		cmd = AP_OFF;
	}

	xQueueOverwrite(command_queue_APCtrl, &cmd);
}


void CTE_WFSAP_change_AP_config(void)
{
	if (command_queue_APCtrl == NULL) return;

	uint8_t cmd = WFSAP_CHANGE_CONFIG;

	xQueueOverwrite(command_queue_APCtrl, &cmd);
}


void UserApp_AP_Controller(void *arg)
{
	DA16X_UNUSED_ARG(arg);

	static uint8_t buffer[1];
	static StaticQueue_t instance;
	command_queue_APCtrl = xQueueCreateStatic(1, 1, buffer, &instance);

	// 程式預設會在啟動後關閉AP(cli ap stop)，所以status當然是false(=off)
	bool ap_status = false;

	while (1)
	{
		uint8_t cmd;
		if (xQueueReceive(command_queue_APCtrl, &cmd, portMAX_DELAY) == pdTRUE)
		{
			switch (cmd)
			{
			case AP_ON:
				PRINTF("AP Controller: start\n");
				da16x_cli_reply("ap start", NULL, NULL);
				ap_status = true;
				break;

			case AP_OFF:
				PRINTF("AP Controller: stop\n");
				da16x_cli_reply("ap stop", NULL, NULL);
				ap_status = false;
				break;

			case WFSAP_CHANGE_CONFIG:
				PRINTF("AP Controller: change AP\n");
				if (ap_status)
				{
					PRINTF("> restart AP...\n");
					da16x_cli_reply("ap stop", NULL, NULL);
					vTaskDelay(2);
					da16x_cli_reply("ap start", NULL, NULL);
				}
				break;
			}
		}
	}
}


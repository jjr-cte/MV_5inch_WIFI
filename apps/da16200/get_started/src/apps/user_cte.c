#include "sdk_type.h"
#include "user_main.h"
#include "common_def.h"
#include "common_uart.h"
#include "util_api.h"
#include "sys_specific.h"
#include "atcmd.h"
#include "user_cte.h"



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
			if ((sz = snprintf(itr, remain, "  [%04X]", lno)) > 0)
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


void CTE_OTA_GPIO_INIT(void)
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


void CTE_OTA_NXP_BOOTMODE(const CTE_OTA_NXP_BOOTMODE_T mode)
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
    case NXP_BOOT_OTA:
    	pin_value = GPIO_PIN3;	//high
        break;
    default :
        configASSERT(0);
        return;
    }

	GPIO_WRITE(gpioA, GPIO_PIN3, &pin_value, sizeof(pin_value));
}


void CTE_OTA_NXP_RESET(void)
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


bool CTE_OTA_NXP_ALIVE()
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
    int     err_code = AT_CMD_ERR_CMD_OK;

    CTE_DBG("%s()\n", __func__);

    if (argc != 2) {
        err_code = AT_CMD_ERR_UNKNOWN;
    }
    else {
        CTE_DBG("argv[1] = %s\n", argv[1]);

        if (0 == strcasecmp(argv[1], "on")) {
            /* RF ON */
            wifi_cs_rf_cntrl(0);
        }
        else if (0 == strcasecmp(argv[1], "off")) {
            /* RF OFF */
            wifi_cs_rf_cntrl(1);
        }
        else {
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
    	CTE_OTA_Nxp_erase_answer(true);
    }
    else if (0 == strcasecmp(argv[0] + 3, "ERASENG"))
    {
    	CTE_OTA_Nxp_erase_answer(false);
    }
    else if (0 == strcasecmp(argv[0] + 3, "MD5OK"))
    {
    	CTE_OTA_MD5_result(true);
    }
    else if (0 == strcasecmp(argv[0] + 3, "MD5NG"))
    {
    	CTE_OTA_MD5_result(false);
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


static ota_type_t       _ota_type = OTA_TYPE_NULL;

void ota_type_set(ota_type_t t)
{
    _ota_type = t;
}

ota_type_t ota_type_get(void)
{
    return _ota_type;
}

void ota_type_invalidate(void)
{
	_ota_type = OTA_TYPE_NULL;
}


////////////////////////////////////////////////////////////////////////////////


/*
 * under AT-CMD task, priority OS_TASK_PRIORITY_USER + 6
 * call path : atcmd_task() -> monitor_atcmd() -> nxpcmd_parser()
 */
void nxpcmd_parser(const uint8_t *in_buf)
{
	switch (in_buf[1])
	{
	case 0xA1:
		//special process for 5A A1 5A A4
		if (in_buf[2] == 0x5A && in_buf[3] == 0xA4)
		{
			CTE_OTA_Nxp_flash_finished();
		}
		else
		{
			CTE_OTA_Nxp_write_block_ok();
		}
		break;

	case 0xA2:
		CTE_DBG(RED_COLOR "NAK\n" CLEAR_COLOR);
		CTE_OTA_Nxp_write_block_abort();
		break;

	case 0xA3:
		CTE_DBG(RED_COLOR "CRC error\n" CLEAR_COLOR);
		CTE_OTA_Nxp_write_block_abort();
		break;

	case 0xA4:
		//special process for 5A A1 5A A4
		CTE_DBG(YELLOW_COLOR "separated A4\n" CLEAR_COLOR);
		CTE_OTA_Nxp_flash_finished();
		break;

	case 0xFE:
		CTE_DBG(RED_COLOR "NXP not received\n" CLEAR_COLOR);
		CTE_OTA_Nxp_write_block_abort();
		break;

	case 0xFF:
		CTE_DBG(RED_COLOR "NXP stopped\n" CLEAR_COLOR);
		CTE_OTA_Nxp_write_block_abort();
		break;

	default:
		CTE_DBG_XXX();
		CTE_DBG("%s(): unknown %02X\n", in_buf[1]);
		break;
	}
}


////////////////////////////////////////////////////////////////////////////////


static SemaphoreHandle_t sem_drmm = NULL;

static void init_sem_drmm()
{
	if (sem_drmm != NULL) return;

	static StaticSemaphore_t instance;
	sem_drmm = xSemaphoreCreateBinaryStatic(&instance);
}

void monitor_dashboard_alive()
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

#define INITIAL_WAIT_TIME	pdMS_TO_TICKS(2800)	// 一開始等待MCU重開，alive訊號線放掉的時間
#define POLLING_INTERVAL	pdMS_TO_TICKS(400)	// 每次polling的間隔
#define MAX_POLLING_RETRY	6					// 總polling次數

		bool dashboard_alive = false;
		for (int i = 0; i < MAX_POLLING_RETRY; i++)
		{
			if (i == 0) vTaskDelay(INITIAL_WAIT_TIME);
			else vTaskDelay(POLLING_INTERVAL);

			if (CTE_OTA_NXP_ALIVE())
			{
				dashboard_alive = true;
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
			CTE_DBG_PRINT_TS();
			CTE_DBG("DRMM: enter DRM\n");
			CTE_Enter_Dashboard_Recovery_Mode();
		}
	}
}

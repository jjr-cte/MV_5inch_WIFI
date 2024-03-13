#include "sdk_type.h"
#include "user_main.h"
#include "common_def.h"
#include "common_uart.h"
#include "util_api.h"
#include "sys_specific.h"
#include "atcmd.h"
#include "user_cte.h"


void CTE_OTA_NXP_INIT(void)
{
    CTE_OTA_NXP_BOOTMODE(NXP_BOOT_NORMAL);
    _GPIO_RETAIN_HIGH(GPIO_UNIT_A, GPIO_PIN2);

    _GPIO_RETAIN_HIGH(GPIO_UNIT_A, GPIO_PIN10);
    _GPIO_RETAIN_HIGH(GPIO_UNIT_A, GPIO_PIN11);
}

void CTE_OTA_NXP_BOOTMODE(const CTE_OTA_NXP_BOOTMODE_T mode)
{
    CTE_DBG("%s(), mode = %d\n", __func__, mode);

    switch (mode) {
        case NXP_BOOT_NORMAL :
            _GPIO_RETAIN_LOW(GPIO_UNIT_A, GPIO_PIN3);
            break;
        case NXP_BOOT_OTA :
            _GPIO_RETAIN_HIGH(GPIO_UNIT_A, GPIO_PIN3);
            break;
        default :
            CTE_DBG_XXX();
            break;
    }
}

void CTE_OTA_NXP_RESET(void)
{
    CTE_DBG("%s()\n", __func__);

    _GPIO_RETAIN_HIGH(GPIO_UNIT_A, GPIO_PIN2);
    vTaskDelay(pdMS_TO_TICKS(100));
    _GPIO_RETAIN_LOW(GPIO_UNIT_A, GPIO_PIN2);
    vTaskDelay(pdMS_TO_TICKS(100));
    _GPIO_RETAIN_HIGH(GPIO_UNIT_A, GPIO_PIN2);
    vTaskDelay(pdMS_TO_TICKS(100));
}

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

    CTE_DBG("%s()\n", __func__);

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


#define XMODEM_POLY     0x1021
#define CRC_TABLE_SIZE  256

static uint16_t     crc_table[CRC_TABLE_SIZE];

void cte_generate_crc_table(void)
{
    for (uint16_t i = 0; i < CRC_TABLE_SIZE; i++) {
        uint16_t crc = i << 8;

        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ XMODEM_POLY;
            }
            else {
                crc <<= 1;
            }
        }

        crc_table[i] = crc;
    }
}

static uint16_t calc_crc16_byte(const uint16_t crc, const uint8_t byte)
{
    return (crc << 8) ^ crc_table[(crc >> 8) ^ byte];
}


static void NXP_OTA_UART_WRITE(const void *data, size_t len)
{
#if defined ( __ATCMD_IF_UART2__ )
    HANDLE atcmd_uart = uart2;
#else
    HANDLE atcmd_uart = uart1;
#endif // (__ATCMD_IF_UART2__)

    UART_WRITE(atcmd_uart, (VOID *)data, (UINT32)len);
}



void nxp_answer_ack()
{
	uint8_t buf[2] = { 0x5A, 0xA1 };
	NXP_OTA_UART_WRITE(buf, sizeof(buf));
}


/*
 *   t->length
 *    |
 *    v
 * |<- ->|<-2->|<-2->|<-- length          -->|
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * | CMD | LEN | CRC | DATA                  |
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |5A A5|00 02|68 E2|                       |
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 */
void nxp_ota_msg_data(const void *data, size_t length)
{
    uint8_t buf[6] = { 0 };
    const uint8_t *raw = (const uint8_t *)data;

    CTE_DBG("%s(): length = %u\n", __func__, length);

    if (length > OTA_CTE_NXP_LEN) {
        CTE_DBG_XXX();
        CTE_DBG("memory address = 0x%08X, length = %u\n", data, length);
        length = OTA_CTE_NXP_LEN;
    }

    size_t idx = 0;
    buf[idx++] = 0x5A;
    buf[idx++] = 0xA5;
    buf[idx++] = (uint8_t) ( length        & 0xFF);
    buf[idx++] = (uint8_t) ((length >>  8) & 0xFF);

    uint16_t crc = 0;
    for (size_t i = 0; i < 4; i++) {
        crc = calc_crc16_byte(crc, buf[i]);
    }

    for (size_t i = 0; i < length; i++) {
        crc = calc_crc16_byte(crc, raw[i]);
    }

    buf[idx++] = (uint8_t) ( crc        & 0xFF);
    buf[idx++] = (uint8_t) ((crc >>  8) & 0xFF);

    memory_dump(buf, sizeof(buf));

    NXP_OTA_UART_WRITE(buf, sizeof(buf));
    NXP_OTA_UART_WRITE(raw, length);
}



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

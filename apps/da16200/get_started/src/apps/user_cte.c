#include "sdk_type.h"

#include "user_main.h"
#include "common_def.h"
#include "common_uart.h"
#include "util_api.h"

#include "sys_specific.h"

#include "atcmd.h"

#include "user_cte.h"
#include "nxp_fw_database.h" //Timmy

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
            CTE_DBG("XXX !!\n");
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

#define XMODEM_POLY     0x1021
#define CRC_TABLE_SIZE  256

static uint16_t     crc_table[CRC_TABLE_SIZE];

void generate_crc_table(void)
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

void NXP_OTA_UART_WRITE_N(const uint8_t *x, size_t n)
{
    puts_UART(g_ota_mcu_uart_idx, (char *)(x), n);

    CTE_DBG_PRINT_TS();
    CTE_DBG("-> NXP\n");

#if 0
#ifdef CTE_DBG_DUMP_PARTIAL
    if (n > 32)
        memory_dump((char *)(x), 32);
    else
#endif
        memory_dump((char *)(x), n);
#endif
}

typedef enum _nxp_ota_cmd_t {
    NXP_OTA_COMMAND,
    NXP_OTA_DATAPAYLOAD,
} nxp_ota_cmd_t;

typedef enum _nxp_ota_cmd_sub_t {
    NXP_OTA_SUB_FLASH_ERASE,
    NXP_OTA_SUB_FILL_MEMORY,
    NXP_OTA_SUB_CONFIG_MEMORY,
    NXP_OTA_SUB_GET_PROPERTY,
    NXP_OTA_SUB_WRITE_MEMORY,
} nxp_ota_cmd_sub_t;

typedef struct _nxp_ota_cmd_pair_t {
    const uint8_t   *data;
    const uint32_t  length;
} nxp_ota_cmd_pair_t;

static const nxp_ota_cmd_pair_t nxp_ota_cmd_pair[] = {
    [ NXP_OTA_COMMAND ] = {
            .data   = (const uint8_t[]) { 0x5a, 0xa4 },
            .length = 2
        },
    [ NXP_OTA_DATAPAYLOAD ] = {
            .data   = (const uint8_t[]) { 0x5a, 0xa5 },
            .length = 2
        },
};

static const nxp_ota_cmd_pair_t nxp_ota_cmd_sub_pair[] = {
    [ NXP_OTA_SUB_FLASH_ERASE ] = {
            .data   = (const uint8_t[]) { 0x02, 0x00, 0x00, 0x03 },
            .length = 4
        },
    [ NXP_OTA_SUB_FILL_MEMORY ] = {
            .data   = (const uint8_t[]) { 0x05, 0x00, 0x00, 0x03 },
            .length = 4
        },
    [ NXP_OTA_SUB_CONFIG_MEMORY ] = {
            .data   = (const uint8_t[]) { 0x11, 0x00, 0x00, 0x02, 0x09, 0x00, 0x00, 0x00 },
            .length = 8
        },
    [ NXP_OTA_SUB_GET_PROPERTY ] = {
            .data   = (const uint8_t[]) { 0x07, 0x00, 0x00, 0x02 },
            .length = 4
        },
    [ NXP_OTA_SUB_WRITE_MEMORY ] = {
            .data   = (const uint8_t[]) { 0x04, 0x01, 0x00, 0x03 },
            .length = 4
        },
};

uint16_t nxp_ota_msg_calc_crc16(const uint8_t *payload, const uint16_t length)
{
    uint16_t        crc = 0;

    for (size_t i = 0; i < 4; i++) {
        crc = calc_crc16_byte(crc, payload[i]);
    }

    for (size_t i = 0; i < length; i++) {
        crc = calc_crc16_byte(crc, payload[6 + i]);
    }

    return crc;
}

/*
 *   t->length
 *    |
 *    v
 * |<- ->|<-2->|<-2->|<-- length                                  -->|
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * | CMD | LEN | CRC |FLASH ERASE|START ADDR |BYTE COUNT | MEMORY ID |
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |5A A4|10 00|47 1E|02 00 00 03|00 00 00 30|00 80 0A 00|00 00 00 00|
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 */
static uint16_t nxp_ota_msg_flash_erase_region(uint8_t *payload, const uint32_t startaddr, const uint32_t _bytecount, const uint32_t memoryid)
{
    const nxp_ota_cmd_t         type = NXP_OTA_COMMAND;
    const nxp_ota_cmd_sub_t     subtype = NXP_OTA_SUB_FLASH_ERASE;
    const nxp_ota_cmd_pair_t    *t = &nxp_ota_cmd_pair[type];
    const nxp_ota_cmd_pair_t    *st = &nxp_ota_cmd_sub_pair[subtype];
    uint32_t                    bytecount = (_bytecount + 0xFFF) & ~0xFFF;      // 0x1000 alignment
    const uint32_t              length = st->length + sizeof(startaddr) + sizeof(bytecount) + sizeof(memoryid);
    const uint32_t              payload_length = t->length + 2 + 2 + length;
    uint32_t                    idx = 0;
    uint16_t                    crc;

    CTE_DBG("%s(), startaddr = 0x%08X, bytecount = 0x%08X, memoryid = %d\n", __func__, startaddr, bytecount, memoryid);

    memcpy(&payload[idx], t->data, t->length);
    idx += t->length;

    payload[idx++] = (uint8_t) ( length        & 0xFF);
    payload[idx++] = (uint8_t) ((length >>  8) & 0xFF);

    /* reserve for crc16 */
    idx += 2;

    memcpy(&payload[idx], st->data, st->length);
    idx += st->length;

    NXP_OTA_MSG_APPEND(payload, idx, startaddr);
    NXP_OTA_MSG_APPEND(payload, idx, bytecount);
    NXP_OTA_MSG_APPEND(payload, idx, memoryid);

    crc = nxp_ota_msg_calc_crc16(payload, length);

    payload[4] = (uint8_t) ( crc        & 0xFF);
    payload[5] = (uint8_t) ((crc >>  8) & 0xFF);

    return payload_length;
}

/*
 *   t->length
 *    |
 *    v
 * |<- ->|<-2->|<-2->|<-- length                                  -->|
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * | CMD | LEN | CRC |FLASH ERASE|START ADDR |BYTE COUNT | MEMORY ID |
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |5A A4|10 00|0B FA|05 00 00 03|00 30 20 20|04 00 00 00|F0 00 00 0F|
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 */
static uint16_t nxp_ota_msg_fill_memory(uint8_t *payload, const uint32_t startaddr, const uint32_t bytecount, const uint32_t pattern)
{
    const nxp_ota_cmd_t         type = NXP_OTA_COMMAND;
    const nxp_ota_cmd_sub_t     subtype = NXP_OTA_SUB_FILL_MEMORY;
    const nxp_ota_cmd_pair_t    *t = &nxp_ota_cmd_pair[type];
    const nxp_ota_cmd_pair_t    *st = &nxp_ota_cmd_sub_pair[subtype];
    const uint32_t              length = st->length + sizeof(startaddr) + sizeof(bytecount) + sizeof(pattern);
    const uint32_t              payload_length = t->length + 2 + 2 + length;
    uint32_t                    idx = 0;
    uint16_t                    crc;

    CTE_DBG("%s(), startaddr = 0x%08X, bytecount = 0x%08X, pattern = 0x%08X\n", __func__, startaddr, bytecount, pattern);

    memcpy(&payload[idx], t->data, t->length);
    idx += t->length;

    payload[idx++] = (uint8_t) ( length        & 0xFF);
    payload[idx++] = (uint8_t) ((length >>  8) & 0xFF);

    /* reserve for crc16 */
    idx += 2;

    memcpy(&payload[idx], st->data, st->length);
    idx += st->length;

    NXP_OTA_MSG_APPEND(payload, idx, startaddr);
    NXP_OTA_MSG_APPEND(payload, idx, bytecount);
    NXP_OTA_MSG_APPEND(payload, idx, pattern);

    crc = nxp_ota_msg_calc_crc16(payload, length);

    payload[4] = (uint8_t) ( crc        & 0xFF);
    payload[5] = (uint8_t) ((crc >>  8) & 0xFF);

    return payload_length;
}

/*
 *   t->length
 *    |
 *    v
 * |<- ->|<-2->|<-2->|<-- length                      -->|
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * | CMD | LEN | CRC |     CONFIG MEMORY     |START ADDR |
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |5A A4|0C 00|DE F6|11 00 00 02 09 00 00 00|00 30 20 20|
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 */
static uint16_t nxp_ota_msg_config_memory(uint8_t *payload, const uint32_t startaddr)
{
    const nxp_ota_cmd_t         type = NXP_OTA_COMMAND;
    const nxp_ota_cmd_sub_t     subtype = NXP_OTA_SUB_CONFIG_MEMORY;
    const nxp_ota_cmd_pair_t    *t = &nxp_ota_cmd_pair[type];
    const nxp_ota_cmd_pair_t    *st = &nxp_ota_cmd_sub_pair[subtype];
    const uint32_t              length = st->length + sizeof(startaddr);
    const uint32_t              payload_length = t->length + 2 + 2 + length;
    uint32_t                    idx = 0;
    uint16_t                    crc;

    CTE_DBG("%s(), startaddr = 0x%08X\n", __func__, startaddr);

    memcpy(&payload[idx], t->data, t->length);
    idx += t->length;

    payload[idx++] = (uint8_t) ( length        & 0xFF);
    payload[idx++] = (uint8_t) ((length >>  8) & 0xFF);

    /* reserve for crc16 */
    idx += 2;

    memcpy(&payload[idx], st->data, st->length);
    idx += st->length;

    NXP_OTA_MSG_APPEND(payload, idx, startaddr);

    crc = nxp_ota_msg_calc_crc16(payload, length);

    payload[4] = (uint8_t) ( crc        & 0xFF);
    payload[5] = (uint8_t) ((crc >>  8) & 0xFF);

    return payload_length;
}

/*
 *   t->length
 *    |
 *    v
 * |<- ->|<-2->|<-2->|<-- length                      -->|
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * | CMD | LEN | CRC | GET PROP  |    TAG    | MEMORY ID |
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |5A A4|0C 00|37 A2|07 00 00 02|0B 00 00 00|00 00 00 00|
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 */
static uint16_t nxp_ota_msg_get_property(uint8_t *payload, const uint32_t tag, const uint32_t memoryid)
{
    const nxp_ota_cmd_t         type = NXP_OTA_COMMAND;
    const nxp_ota_cmd_sub_t     subtype = NXP_OTA_SUB_GET_PROPERTY;
    const nxp_ota_cmd_pair_t    *t = &nxp_ota_cmd_pair[type];
    const nxp_ota_cmd_pair_t    *st = &nxp_ota_cmd_sub_pair[subtype];
    const uint32_t              length = st->length + sizeof(tag) + sizeof(memoryid);
    const uint32_t              payload_length = t->length + 2 + 2 + length;
    uint32_t                    idx = 0;
    uint16_t                    crc;

    CTE_DBG("%s(), tag = 0x%02X, memoryid = %d\n", __func__, tag, memoryid);

    memcpy(&payload[idx], t->data, t->length);
    idx += t->length;

    payload[idx++] = (uint8_t) ( length        & 0xFF);
    payload[idx++] = (uint8_t) ((length >>  8) & 0xFF);

    /* reserve for crc16 */
    idx += 2;

    memcpy(&payload[idx], st->data, st->length);
    idx += st->length;

    NXP_OTA_MSG_APPEND(payload, idx, tag);
    NXP_OTA_MSG_APPEND(payload, idx, memoryid);

    crc = nxp_ota_msg_calc_crc16(payload, length);

    payload[4] = (uint8_t) ( crc        & 0xFF);
    payload[5] = (uint8_t) ((crc >>  8) & 0xFF);

    return payload_length;
}

/*
 *   t->length
 *    |
 *    v
 * |<- ->|<-2->|<-2->|<-- length                                  -->|
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * | CMD | LEN | CRC | WRITE MEM |START ADDR |BYTE COUNT | MEMORY ID |
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |5A A4|10 00|A9 BB|04 01 00 03|00 10 00 30|DC 66 0A 00|00 00 00 00|
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 */
static uint16_t nxp_ota_msg_write_memory(uint8_t *payload, const uint32_t startaddr, const uint32_t bytecount, const uint32_t memoryid)
{
    const nxp_ota_cmd_t         type = NXP_OTA_COMMAND;
    const nxp_ota_cmd_sub_t     subtype = NXP_OTA_SUB_WRITE_MEMORY;
    const nxp_ota_cmd_pair_t    *t = &nxp_ota_cmd_pair[type];
    const nxp_ota_cmd_pair_t    *st = &nxp_ota_cmd_sub_pair[subtype];
    const uint32_t              length = st->length + sizeof(startaddr) + sizeof(bytecount) + sizeof(memoryid);
    const uint32_t              payload_length = t->length + 2 + 2 + length;
    uint32_t                    idx = 0;
    uint16_t                    crc;

    CTE_DBG("%s(), startaddr = 0x%08X, bytecount = 0x%08X, memoryid = %d\n", __func__, startaddr, bytecount, memoryid);

    memcpy(&payload[idx], t->data, t->length);
    idx += t->length;

    payload[idx++] = (uint8_t) ( length        & 0xFF);
    payload[idx++] = (uint8_t) ((length >>  8) & 0xFF);

    /* reserve for crc16 */
    idx += 2;

    memcpy(&payload[idx], st->data, st->length);
    idx += st->length;

    NXP_OTA_MSG_APPEND(payload, idx, startaddr);
    NXP_OTA_MSG_APPEND(payload, idx, bytecount);
    NXP_OTA_MSG_APPEND(payload, idx, memoryid);

    crc = nxp_ota_msg_calc_crc16(payload, length);

    payload[4] = (uint8_t) ( crc        & 0xFF);
    payload[5] = (uint8_t) ((crc >>  8) & 0xFF);

    return payload_length;
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
static void nxp_ota_msg_data_payload(uint8_t *raw, size_t length)
{
    uint8_t                     buf[6] = { 0 };
    const nxp_ota_cmd_t         type = NXP_OTA_DATAPAYLOAD;
    const nxp_ota_cmd_pair_t    *t = &nxp_ota_cmd_pair[type];
    uint32_t                    idx = 0;
    uint16_t                    crc = 0;

    CTE_DBG("%s(), length = %d\n", __func__, length);

    if (length > 512) {
        CTE_DBG("XXX !! raw = 0x%08X, length = %d\n", raw, length);
        length = 512;
    }

    memcpy(&buf[idx], t->data, t->length);
    idx += t->length;

    buf[idx++] = (uint8_t) ( length        & 0xFF);
    buf[idx++] = (uint8_t) ((length >>  8) & 0xFF);

    for (size_t i = 0; i < 4; i++) {
        crc = calc_crc16_byte(crc, buf[i]);
    }

    for (size_t i = 0; i < length; i++) {
        crc = calc_crc16_byte(crc, raw[i]);
    }

    buf[idx++] = (uint8_t) ( crc        & 0xFF);
    buf[idx++] = (uint8_t) ((crc >>  8) & 0xFF);

    NXP_OTA_UART_WRITE(buf);
    NXP_OTA_UART_WRITE_N(raw, length);
}

//Timmy
static int p_script_name = 0;

void nxp_script_select(int script_name)
{
    p_script_name = script_name;
    uint8_t _5AA6[] = {0x5a, 0xa6};
    puts_UART(g_ota_mcu_uart_idx, (char *)_5AA6, sizeof(_5AA6));
}

void nxp_load_fw(int script_name)
{
    p_script_name = script_name;
    switch (p_script_name)
    {
        case pFLASHLOADER:   {puts_UART(g_ota_mcu_uart_idx, (char *)NXP_RT1176_IVT[0],    sizeof(NXP_RT1176_IVT[0]));}      break;
        case pFWCONFIG:      {puts_UART(g_ota_mcu_uart_idx, (char *)NXP_RT1176_CONFIG[0], sizeof(NXP_RT1176_CONFIG[0]));}   break;
        case pFWPACKAGE:     {puts_UART(g_ota_mcu_uart_idx, (char *)NXP_RT1176_FW[0],     sizeof(NXP_RT1176_FW[0]));}       break;
    }
}

UINT nxpcmd_parser(char *in_buf)
{
    uint8_t _5AA15AA4[]                = {0x5a, 0xa1, 0x5a, 0xa4};
    uint8_t _5AA1[]                    = {0x5a, 0xa1};
    uint8_t _5AA7[]                    = {0x5a, 0xa7};
    uint8_t GetProperty01[]            = {0x5A, 0xA4, 0x0C, 0x00, 0x4B, 0x33, 0x07, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t FillMemory1[]              = {0x5A, 0xA4, 0x10, 0x00, 0x3B, 0xD2, 0x05, 0x00, 0x00, 0x03, 0x00, 0x20, 0x20, 0x20, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x90, 0xCF};
    uint8_t ConfigureMemory[]          = {0x5A, 0xA4, 0x0C, 0x00, 0xBD, 0xB5, 0x11, 0x00, 0x00, 0x02, 0x09, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 0x20};
    uint8_t FillMemory2[]              = {0x5A, 0xA4, 0x10, 0x00, 0xC6, 0x72, 0x05, 0x00, 0x00, 0x03, 0x00, 0x20, 0x20, 0x20, 0x04, 0x00, 0x00, 0x00, 0x07, 0x02, 0x00, 0xC0};
    uint8_t FillMemory3[]              = {0x5A, 0xA4, 0x10, 0x00, 0x13, 0x99, 0x05, 0x00, 0x00, 0x03, 0x04, 0x20, 0x20, 0x20, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t FlashEraseRegionCode1000[] = {0x5A, 0xA4, 0x10, 0x00, 0x5D, 0x92, 0x02, 0x00, 0x00, 0x03, 0x00, 0x20, 0x00, 0x30, 0x00, 0x00, 0xEE, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t ConfigureMemoryCode[]      = {0x5A, 0xA4, 0x0C, 0x00, 0xDE, 0xF6, 0x11, 0x00, 0x00, 0x02, 0x09, 0x00, 0x00, 0x00, 0x00, 0x30, 0x20, 0x20};
    uint8_t GetPropertyCode[]          = {0x5A, 0xA4, 0x0C, 0x00, 0x37, 0xA2, 0x07, 0x00, 0x00, 0x02, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t WriteMemoryCode[]          = {0x5A, 0xA4, 0x10, 0x00, 0xC8, 0x7D, 0x04, 0x01, 0x00, 0x03, 0x00, 0x20, 0x00, 0x30, 0x8F, 0x9E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	if (strncmp((const char *)in_buf, (char *)_5AA7, sizeof(_5AA7)) == 0)
    {
	    switch (p_script_name)
	    {
	        case pGetProperty01_1:          {puts_UART(g_ota_mcu_uart_idx, (char *)GetProperty01, sizeof(GetProperty01));}                       break;
	        case pGetProperty01_2:          {puts_UART(g_ota_mcu_uart_idx, (char *)GetProperty01, sizeof(GetProperty01));}                       break;
	        case pFillMemory1:              {puts_UART(g_ota_mcu_uart_idx, (char *)FillMemory1, sizeof(FillMemory1));}                           break;
	        case pFillMemory2:              {puts_UART(g_ota_mcu_uart_idx, (char *)FillMemory2, sizeof(FillMemory2));}                           break;
	        case pFillMemory3:              {puts_UART(g_ota_mcu_uart_idx, (char *)FillMemory3, sizeof(FillMemory3));}                           break;
	        case pConfigureMemory_2:        {puts_UART(g_ota_mcu_uart_idx, (char *)ConfigureMemory, sizeof(ConfigureMemory));}                   break;
	        case pFlashEraseRegionCode1000: {puts_UART(g_ota_mcu_uart_idx, (char *)FlashEraseRegionCode1000, sizeof(FlashEraseRegionCode1000));} break;
	        case pConfigureMemoryCode:      {puts_UART(g_ota_mcu_uart_idx, (char *)ConfigureMemoryCode, sizeof(ConfigureMemoryCode));}           break;
	        case pGetPropertyCode:          {puts_UART(g_ota_mcu_uart_idx, (char *)GetPropertyCode, sizeof(GetPropertyCode));}                   break;
	        case pWriteMemoryCode:          {puts_UART(g_ota_mcu_uart_idx, (char *)WriteMemoryCode, sizeof(WriteMemoryCode));}                   break;
	    }
    }
    else if (strncmp((const char *)in_buf, (char *)_5AA15AA4, sizeof(_5AA15AA4)) == 0)
    {
        puts_UART(g_ota_mcu_uart_idx, (char *)_5AA1, sizeof(_5AA1));
        vTaskDelay(8);

        switch (p_script_name)
        {
            case pGetProperty01_1:          {nxp_load_fw(pFLASHLOADER);}                    break;
            case pGetProperty01_2:          {nxp_script_select(pFillMemory1);}              break;
            case pFillMemory1:              {nxp_script_select(pFillMemory2);}              break;
            case pFillMemory2:              {nxp_script_select(pFillMemory3);}              break;
            case pFillMemory3:              {nxp_script_select(pConfigureMemory_2);}        break;
            case pConfigureMemory_2:        {nxp_script_select(pFlashEraseRegionCode1000);} break;
            case pFlashEraseRegionCode1000: {nxp_script_select(pConfigureMemoryCode);}      break;
            case pConfigureMemoryCode:      {nxp_script_select(pGetPropertyCode);}          break;
            case pGetPropertyCode:          {nxp_script_select(pWriteMemoryCode);}          break;
            case pWriteMemoryCode:          {nxp_load_fw(pFWCONFIG);}                       break;
        }
    }
    else if (strncmp((const char *)in_buf, (char *)_5AA1, sizeof(_5AA1)) == 0)
    {
        switch (p_script_name)
        {
            case pFLASHLOADER:
            {
                static uint32_t     idx = 0;
                size_t              sz = ARRAY_SIZE(NXP_RT1176_IVT);

                idx++;

                if (idx < (sz - 1)) {
                	puts_UART(g_ota_mcu_uart_idx, (char *)NXP_RT1176_IVT[idx], sizeof(NXP_RT1176_IVT[idx]));
                }
                else if (idx == (sz - 1)) {
                	puts_UART(g_ota_mcu_uart_idx, (char *)NXP_RT1176_IVT[idx], 40);
                }
                else {
                    vTaskDelay(300);
                    idx = 0;
                    nxp_script_select(pGetProperty01_2);
                }
            }
            break;

            case pFWCONFIG:
            {
                static uint32_t     idx = 0;
                size_t              sz = ARRAY_SIZE(NXP_RT1176_CONFIG);

                idx++;

                if (idx < sz) {
                    nxp_ota_msg_data_payload((uint8_t *)NXP_RT1176_CONFIG[idx], sizeof(NXP_RT1176_CONFIG[idx]));
                }
                else {
                    vTaskDelay(300);
                    idx = 0;
                    nxp_load_fw(pFWPACKAGE);
                }
            }
            break;

            case pFWPACKAGE:
            {
                static uint32_t     idx = 0;
                size_t              sz = ARRAY_SIZE(NXP_RT1176_FW);

                idx++;

                if (idx < (sz - 1)) {
                	puts_UART(g_ota_mcu_uart_idx, (char *)NXP_RT1176_FW[idx], sizeof(NXP_RT1176_FW[idx]));
                }
                else if (idx == (sz - 1)) {
                	puts_UART(g_ota_mcu_uart_idx, (char *)NXP_RT1176_FW[idx], 150);
                }
                else {
                    vTaskDelay(300);
                    idx = 0;
                }
            }
            break;
        }
    }
    return 0;
}

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

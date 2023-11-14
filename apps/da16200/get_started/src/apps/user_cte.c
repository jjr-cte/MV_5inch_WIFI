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
        case pFLASHLOADER:   {puts_UART(g_ota_mcu_uart_idx, (char *)IVT000, sizeof(IVT000));}       break;
        case pFWPACKAGE:     {puts_UART(g_ota_mcu_uart_idx, (char *)FW0000, sizeof(FW0000));}       break;
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
            case pWriteMemoryCode:          {nxp_load_fw(pFWPACKAGE);}                      break;
        }
    }
    else if (strncmp((const char *)in_buf, (char *)_5AA1, sizeof(_5AA1)) == 0)
    {
        switch (p_script_name)
        {
            case pFLASHLOADER:
            {
                static uint8_t idx = 0;
                idx++;
                switch (idx)
                {
                    case   1: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT001, sizeof(IVT001));} break;
                    case   2: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT002, sizeof(IVT002));} break;
                    case   3: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT003, sizeof(IVT003));} break;
                    case   4: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT004, sizeof(IVT004));} break;
                    case   5: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT005, sizeof(IVT005));} break;
                    case   6: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT006, sizeof(IVT006));} break;
                    case   7: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT007, sizeof(IVT007));} break;
                    case   8: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT008, sizeof(IVT008));} break;
                    case   9: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT009, sizeof(IVT009));} break;
                    case  10: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT010, sizeof(IVT010));} break;
                    case  11: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT011, sizeof(IVT011));} break;
                    case  12: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT012, sizeof(IVT012));} break;
                    case  13: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT013, sizeof(IVT013));} break;
                    case  14: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT014, sizeof(IVT014));} break;
                    case  15: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT015, sizeof(IVT015));} break;
                    case  16: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT016, sizeof(IVT016));} break;
                    case  17: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT017, sizeof(IVT017));} break;
                    case  18: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT018, sizeof(IVT018));} break;
                    case  19: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT019, sizeof(IVT019));} break;
                    case  20: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT020, sizeof(IVT020));} break;
                    case  21: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT021, sizeof(IVT021));} break;
                    case  22: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT022, sizeof(IVT022));} break;
                    case  23: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT023, sizeof(IVT023));} break;
                    case  24: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT024, sizeof(IVT024));} break;
                    case  25: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT025, sizeof(IVT025));} break;
                    case  26: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT026, sizeof(IVT026));} break;
                    case  27: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT027, sizeof(IVT027));} break;
                    case  28: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT028, sizeof(IVT028));} break;
                    case  29: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT029, sizeof(IVT029));} break;
                    case  30: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT030, sizeof(IVT030));} break;
                    case  31: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT031, sizeof(IVT031));} break;
                    case  32: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT032, sizeof(IVT032));} break;
                    case  33: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT033, sizeof(IVT033));} break;
                    case  34: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT034, sizeof(IVT034));} break;
                    case  35: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT035, sizeof(IVT035));} break;
                    case  36: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT036, sizeof(IVT036));} break;
                    case  37: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT037, sizeof(IVT037));} break;
                    case  38: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT038, sizeof(IVT038));} break;
                    case  39: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT039, sizeof(IVT039));} break;
                    case  40: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT040, sizeof(IVT040));} break;
                    case  41: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT041, sizeof(IVT041));} break;
                    case  42: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT042, sizeof(IVT042));} break;
                    case  43: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT043, sizeof(IVT043));} break;
                    case  44: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT044, sizeof(IVT044));} break;
                    case  45: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT045, sizeof(IVT045));} break;
                    case  46: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT046, sizeof(IVT046));} break;
                    case  47: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT047, sizeof(IVT047));} break;
                    case  48: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT048, sizeof(IVT048));} break;
                    case  49: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT049, sizeof(IVT049));} break;
                    case  50: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT050, sizeof(IVT050));} break;
                    case  51: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT051, sizeof(IVT051));} break;
                    case  52: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT052, sizeof(IVT052));} break;
                    case  53: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT053, sizeof(IVT053));} break;
                    case  54: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT054, sizeof(IVT054));} break;
                    case  55: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT055, sizeof(IVT055));} break;
                    case  56: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT056, sizeof(IVT056));} break;
                    case  57: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT057, sizeof(IVT057));} break;
                    case  58: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT058, sizeof(IVT058));} break;
                    case  59: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT059, sizeof(IVT059));} break;
                    case  60: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT060, sizeof(IVT060));} break;
                    case  61: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT061, sizeof(IVT061));} break;
                    case  62: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT062, sizeof(IVT062));} break;
                    case  63: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT063, sizeof(IVT063));} break;
                    case  64: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT064, sizeof(IVT064));} break;
                    case  65: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT065, sizeof(IVT065));} break;
                    case  66: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT066, sizeof(IVT066));} break;
                    case  67: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT067, sizeof(IVT067));} break;
                    case  68: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT068, sizeof(IVT068));} break;
                    case  69: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT069, sizeof(IVT069));} break;
                    case  70: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT070, sizeof(IVT070));} break;
                    case  71: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT071, sizeof(IVT071));} break;
                    case  72: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT072, sizeof(IVT072));} break;
                    case  73: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT073, sizeof(IVT073));} break;
                    case  74: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT074, sizeof(IVT074));} break;
                    case  75: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT075, sizeof(IVT075));} break;
                    case  76: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT076, sizeof(IVT076));} break;
                    case  77: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT077, sizeof(IVT077));} break;
                    case  78: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT078, sizeof(IVT078));} break;
                    case  79: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT079, sizeof(IVT079));} break;
                    case  80: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT080, sizeof(IVT080));} break;
                    case  81: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT081, sizeof(IVT081));} break;
                    case  82: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT082, sizeof(IVT082));} break;
                    case  83: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT083, sizeof(IVT083));} break;
                    case  84: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT084, sizeof(IVT084));} break;
                    case  85: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT085, sizeof(IVT085));} break;
                    case  86: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT086, sizeof(IVT086));} break;
                    case  87: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT087, sizeof(IVT087));} break;
                    case  88: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT088, sizeof(IVT088));} break;
                    case  89: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT089, sizeof(IVT089));} break;
                    case  90: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT090, sizeof(IVT090));} break;
                    case  91: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT091, sizeof(IVT091));} break;
                    case  92: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT092, sizeof(IVT092));} break;
                    case  93: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT093, sizeof(IVT093));} break;
                    case  94: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT094, sizeof(IVT094));} break;
                    case  95: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT095, sizeof(IVT095));} break;
                    case  96: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT096, sizeof(IVT096));} break;
                    case  97: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT097, sizeof(IVT097));} break;
                    case  98: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT098, sizeof(IVT098));} break;
                    case  99: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT099, sizeof(IVT099));} break;
                    case 100: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT100, sizeof(IVT100));} break;
                    case 101: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT101, sizeof(IVT101));} break;
                    case 102: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT102, sizeof(IVT102));} break;
                    case 103: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT103, sizeof(IVT103));} break;
                    case 104: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT104, sizeof(IVT104));} break;
                    case 105: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT105, sizeof(IVT105));} break;
                    case 106: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT106, sizeof(IVT106));} break;
                    case 107: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT107, sizeof(IVT107));} break;
                    case 108: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT108, sizeof(IVT108));} break;
                    case 109: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT109, sizeof(IVT109));} break;
                    case 110: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT110, sizeof(IVT110));} break;
                    case 111: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT111, sizeof(IVT111));} break;
                    case 112: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT112, sizeof(IVT112));} break;
                    case 113: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT113, sizeof(IVT113));} break;
                    case 114: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT114, sizeof(IVT114));} break;
                    case 115: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT115, sizeof(IVT115));} break;
                    case 116: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT116, sizeof(IVT116));} break;
                    case 117: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT117, sizeof(IVT117));} break;
                    case 118: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT118, sizeof(IVT118));} break;
                    case 119: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT119, sizeof(IVT119));} break;
                    case 120: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT120, sizeof(IVT120));} break;
                    case 121: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT121, sizeof(IVT121));} break;
                    case 122: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT122, sizeof(IVT122));} break;
                    case 123: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT123, sizeof(IVT123));} break;
                    case 124: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT124, sizeof(IVT124));} break;
                    case 125: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT125, sizeof(IVT125));} break;
                    case 126: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT126, sizeof(IVT126));} break;
                    case 127: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT127, sizeof(IVT127));} break;
                    case 128: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT128, sizeof(IVT128));} break;
                    case 129: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT129, sizeof(IVT129));} break;
                    case 130: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT130, sizeof(IVT130));} break;
                    case 131: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT131, sizeof(IVT131));} break;
                    case 132: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT132, sizeof(IVT132));} break;
                    case 133: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT133, sizeof(IVT133));} break;
                    case 134: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT134, sizeof(IVT134));} break;
                    case 135: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT135, sizeof(IVT135));} break;
                    case 136: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT136, sizeof(IVT136));} break;
                    case 137: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT137, sizeof(IVT137));} break;
                    case 138: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT138, sizeof(IVT138));} break;
                    case 139: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT139, sizeof(IVT139));} break;
                    case 140: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT140, sizeof(IVT140));} break;
                    case 141: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT141, sizeof(IVT141));} break;
                    case 142: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT142, sizeof(IVT142));} break;
                    case 143: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT143, sizeof(IVT143));} break;
                    case 144: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT144, sizeof(IVT144));} break;
                    case 145: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT145, sizeof(IVT145));} break;
                    case 146: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT146, sizeof(IVT146));} break;
                    case 147: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT147, sizeof(IVT147));} break;
                    case 148: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT148, sizeof(IVT148));} break;
                    case 149: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT149, sizeof(IVT149));} break;
                    case 150: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT150, sizeof(IVT150));} break;
                    case 151: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT151, sizeof(IVT151));} break;
                    case 152: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT152, sizeof(IVT152));} break;
                    case 153: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT153, sizeof(IVT153));} break;
                    case 154: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT154, sizeof(IVT154));} break;
                    case 155: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT155, sizeof(IVT155));} break;
                    case 156: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT156, sizeof(IVT156));} break;
                    case 157: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT157, sizeof(IVT157));} break;
                    case 158: {puts_UART(g_ota_mcu_uart_idx, (char *)IVT158, sizeof(IVT158));} break;
                    case 159: {vTaskDelay(300); idx = 0; nxp_script_select(pGetProperty01_2);} break;
                }
            }
            break;
            case pFWPACKAGE:
            {
                static uint32_t idx = 0;
                idx++;
                switch (idx)
                {
                    case  1: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0001, sizeof(FW0001));} break;
                    case  2: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0002, sizeof(FW0002));} break;
                    case  3: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0003, sizeof(FW0003));} break;
                    case  4: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0004, sizeof(FW0004));} break;
                    case  5: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0005, sizeof(FW0005));} break;
                    case  6: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0006, sizeof(FW0006));} break;
                    case  7: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0007, sizeof(FW0007));} break;
                    case  8: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0008, sizeof(FW0008));} break;
                    case  9: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0009, sizeof(FW0009));} break;
                    case 10: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0010, sizeof(FW0010));} break;
                    case 11: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0011, sizeof(FW0011));} break;
                    case 12: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0012, sizeof(FW0012));} break;
                    case 13: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0013, sizeof(FW0013));} break;
                    case 14: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0014, sizeof(FW0014));} break;
                    case 15: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0015, sizeof(FW0015));} break;
                    case 16: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0016, sizeof(FW0016));} break;
                    case 17: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0017, sizeof(FW0017));} break;
                    case 18: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0018, sizeof(FW0018));} break;
                    case 19: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0019, sizeof(FW0019));} break;
                    case 20: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0020, sizeof(FW0020));} break;
                    case 21: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0021, sizeof(FW0021));} break;
                    case 22: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0022, sizeof(FW0022));} break;
                    case 23: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0023, sizeof(FW0023));} break;
                    case 24: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0024, sizeof(FW0024));} break;
                    case 25: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0025, sizeof(FW0025));} break;
                    case 26: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0026, sizeof(FW0026));} break;
                    case 27: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0027, sizeof(FW0027));} break;
                    case 28: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0028, sizeof(FW0028));} break;
                    case 29: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0029, sizeof(FW0029));} break;
                    case 30: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0030, sizeof(FW0030));} break;
                    case 31: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0031, sizeof(FW0031));} break;
                    case 32: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0032, sizeof(FW0032));} break;
                    case 33: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0033, sizeof(FW0033));} break;
                    case 34: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0034, sizeof(FW0034));} break;
                    case 35: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0035, sizeof(FW0035));} break;
                    case 36: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0036, sizeof(FW0036));} break;
                    case 37: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0037, sizeof(FW0037));} break;
                    case 38: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0038, sizeof(FW0038));} break;
                    case 39: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0039, sizeof(FW0039));} break;
                    case 40: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0040, sizeof(FW0040));} break;
                    case 41: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0041, sizeof(FW0041));} break;
                    case 42: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0042, sizeof(FW0042));} break;
                    case 43: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0043, sizeof(FW0043));} break;
                    case 44: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0044, sizeof(FW0044));} break;
                    case 45: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0045, sizeof(FW0045));} break;
                    case 46: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0046, sizeof(FW0046));} break;
                    case 47: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0047, sizeof(FW0047));} break;
                    case 48: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0048, sizeof(FW0048));} break;
                    case 49: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0049, sizeof(FW0049));} break;
                    case 50: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0050, sizeof(FW0050));} break;
                    case 51: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0051, sizeof(FW0051));} break;
                    case 52: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0052, sizeof(FW0052));} break;
                    case 53: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0053, sizeof(FW0053));} break;
                    case 54: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0054, sizeof(FW0054));} break;
                    case 55: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0055, sizeof(FW0055));} break;
                    case 56: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0056, sizeof(FW0056));} break;
                    case 57: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0057, sizeof(FW0057));} break;
                    case 58: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0058, sizeof(FW0058));} break;
                    case 59: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0059, sizeof(FW0059));} break;
                    case 60: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0060, sizeof(FW0060));} break;
                    case 61: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0061, sizeof(FW0061));} break;
                    case 62: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0062, sizeof(FW0062));} break;
                    case 63: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0063, sizeof(FW0063));} break;
                    case 64: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0064, sizeof(FW0064));} break;
                    case 65: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0065, sizeof(FW0065));} break;
                    case 66: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0066, sizeof(FW0066));} break;
                    case 67: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0067, sizeof(FW0067));} break;
                    case 68: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0068, sizeof(FW0068));} break;
                    case 69: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0069, sizeof(FW0069));} break;
                    case 70: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0070, sizeof(FW0070));} break;
                    case 71: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0071, sizeof(FW0071));} break;
                    case 72: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0072, sizeof(FW0072));} break;
                    case 73: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0073, sizeof(FW0073));} break;
                    case 74: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0074, sizeof(FW0074));} break;
                    case 75: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0075, sizeof(FW0075));} break;
                    case 76: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0076, sizeof(FW0076));} break;
                    case 77: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0077, sizeof(FW0077));} break;
                    case 78: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0078, sizeof(FW0078));} break;
                    case 79: {puts_UART(g_ota_mcu_uart_idx, (char *)FW0079, sizeof(FW0079));} break;
                }
            }
            break;
        }
    }
    return 0;
}

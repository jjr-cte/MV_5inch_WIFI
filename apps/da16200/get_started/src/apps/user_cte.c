#include "sdk_type.h"

#include "user_main.h"
#include "common_def.h"

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


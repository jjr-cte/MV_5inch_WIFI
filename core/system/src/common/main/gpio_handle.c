/**
 ****************************************************************************************
 *
 * @file gpio_handle.c
 *
 * @brief GPIO polling thread
 *
 * Copyright (c) 2016-2022 Renesas Electronics. All rights reserved.
 *
 * This software ("Software") is owned by Renesas Electronics.
 *
 * By using this Software you agree that Renesas Electronics retains all
 * intellectual property and proprietary rights in and to this Software and any
 * use, reproduction, disclosure or distribution of the Software without express
 * written permission or a license agreement from Renesas Electronics is
 * strictly prohibited. This Software is solely for use on or in conjunction
 * with Renesas Electronics products.
 *
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, THE
 * SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. EXCEPT AS OTHERWISE
 * PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, IN NO EVENT SHALL
 * RENESAS ELECTRONICS BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THE SOFTWARE.
 *
 ****************************************************************************************
 */

#include "FreeRTOSConfig.h"
#include "common_def.h"

#include "da16x_system.h"
#include "gpio.h"


/*
 * Local externel APIs
 */

int     gpio_number = 0;
UINT	run_gpio_interrupt_flag = pdFALSE;

int	factory_rst_btn = FACTORY_BUTTON;
#if defined ( __SUPPORT_EVK_LED__ )
int	factory_rst_led = FACTORY_LED;
#endif	// __SUPPORT_EVK_LED__
int	factory_rst_btn_chk_time = CHECK_TIME_FACTORY_BTN;

int	wps_btn = WPS_BUTTON;
#if defined ( __SUPPORT_EVK_LED__ )
int	wps_led = WPS_LED;
#endif	// __SUPPORT_EVK_LED__
int	wps_btn_chk_time = CHECK_TIME_WPS_BTN;


/* polling_gpio_entry */
#define	POLL_GPIO_NAME		"poll_gpio"
#define	POLL_GPIO_STK_SZ	(1024 / sizeof(unsigned long))

TaskHandle_t	poll_gpio_task = NULL;
char		poll_gpio_stk[POLL_GPIO_STK_SZ] = { 0, };

#if defined ( __SUPPORT_EVK_LED__ )
extern UINT check_factory_button(int btn_gpio_num, int led_gpio_num, int check_time);
#else
extern UINT check_factory_button(int btn_gpio_num, int check_time);
#endif	// __SUPPORT_EVK_LED__

extern UINT factory_reset_default(int reboot_flag);
extern void reboot_func(UINT flag);

#ifdef __SUPPORT_WPS_BTN__
#if defined ( __SUPPORT_EVK_LED__ )
extern UINT check_wps_button(int btn_gpio_num, int led_gpio_num, int check_time);
#else
extern UINT check_wps_button(int btn_gpio_num, int check_time);
#endif	// __SUPPORT_EVK_LED__

extern UINT wps_setup(char *macaddr);
#endif	/* __SUPPORT_WPS_BTN__ */


#if defined ( __SUPPORT_EVK_LED__ )
void config_factory_reset_button(int reset_gpio, int led_gpio, int seconds)
{
	factory_rst_btn = reset_gpio;
	factory_rst_led = led_gpio;
	factory_rst_btn_chk_time = seconds;
}

void config_wps_button(int wps_gpio, int led_gpio, int seconds)
{
	wps_btn = wps_gpio;
	wps_led = led_gpio;
	wps_btn_chk_time = seconds;
}
#else
void config_factory_reset_button(int reset_gpio, int seconds)
{
	factory_rst_btn = reset_gpio;
	factory_rst_btn_chk_time = seconds;
}

void config_wps_button(int wps_gpio, int seconds)
{
	wps_btn = wps_gpio;
	wps_btn_chk_time = seconds;
}
#endif	// __SUPPORT_EVK_LED__

static void polling_gpio_entry(void *param)
{
	DA16X_UNUSED_ARG(param);

#if defined ( __SUPPORT_FACTORY_RESET_BTN__ ) || defined ( __SUPPORT_WPS_BTN__ )
	UINT status = 0;
#endif	// __SUPPORT_FACTORY_RESET_BTN__ || __SUPPORT_WPS_BTN__

	while (1) {
		do {
			vTaskDelay(10);
		} while (run_gpio_interrupt_flag == pdFALSE);


#ifdef	__SUPPORT_FACTORY_RESET_BTN__
		if (gpio_number == factory_rst_btn) {	/* GPIO 7 */
			/* Factory Reset */
#if defined ( __SUPPORT_EVK_LED__ )
			status = check_factory_button(factory_rst_btn, factory_rst_led, factory_rst_btn_chk_time);
#else
			status = check_factory_button(factory_rst_btn, factory_rst_btn_chk_time);
#endif	// __SUPPORT_EVK_LED__

			if (status == 1) {
				factory_reset_default(1);
			} else if (status ==  2) {	/* Reboot */
				reboot_func(2);
			}
		} else
#endif	/* __SUPPORT_FACTORY_RESET_BTN__ */
#ifdef __SUPPORT_WPS_BTN__
			if (gpio_number == wps_btn) {	/* GPIO 6 */
				/* WPS */
#if defined ( __SUPPORT_EVK_LED__ )
				status = check_wps_button(wps_btn, wps_led, wps_btn_chk_time);
#else
				status = check_wps_button(wps_btn, wps_btn_chk_time);
#endif	// __SUPPORT_EVK_LED__

				if (status == 1) {
					wps_setup(NULL);
				}
			}

#endif /* __SUPPORT_WPS_BTN__ */

		gpio_number = 0;
	}
}

void start_gpio_thread(void)
{
	BaseType_t	status;

	if (poll_gpio_stk == NULL) {
		PRINTF(RED_COLOR " [%s] Malloc fail \n" CLEAR_COLOR, __func__);
		return;
	}

    status = xTaskCreate(polling_gpio_entry,
							POLL_GPIO_NAME,
							POLL_GPIO_STK_SZ,
							(void *)NULL,
							OS_TASK_PRIORITY_USER + 1,	// Don't assign as 0 priority
							&poll_gpio_task);

	if (status != pdPASS) {
		PRINTF(RED_COLOR " [%s] polling_gpio thread creation fail (status:%d) \n" CLEAR_COLOR, __func__, status);
	}
}

/*
 * Checking flag if interrupt occurs or not
 */
void gpio_callback(void *param)
{
	gpio_number = (int)param;

	run_gpio_interrupt_flag = pdTRUE;
}

/* EOF */

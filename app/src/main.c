/*
 * SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#include <stdio.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>

#include <epub/epub.h>
#include <ui/ui.h>

LOG_MODULE_REGISTER(main, CONFIG_ZEREADER_LOG_LEVEL);

int main(void)
{
	LOG_DBG("ZEReader started! %s\n", CONFIG_BOARD_TARGET);

	// Initialize the choosen zephyr,display device
	// -> Make the device tree description available for the software part
	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev))
	{
		LOG_ERR("Device not ready, aborting...");
		return 0;
	}

	// Make the FIRST ok zephyr,lvgl-button-input node available to the software part
	static const struct device *lvgl_btn_dev;
	lvgl_btn_dev = DEVICE_DT_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_lvgl_button_input));
	if (!device_is_ready(lvgl_btn_dev))
	{
		LOG_ERR("Device not ready, aborting...");
		return 0;
	}

	context_t context = READING;

	zereader_setup_page();
	zereader_setup_control_buttons(&context);
	zereader_show_logo();
	lv_timer_handler();
	display_blanking_off(display_dev);

	epub_initialize();
	epub_restore_book();

	zereader_clean_page();

	zereader_print_current_page();

	lv_timer_handler();

	while (1)
	{
		uint32_t sleep_ms = lv_timer_handler();

		k_msleep(MIN(sleep_ms, INT32_MAX));
	}

	return 0;
}
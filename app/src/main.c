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
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/input/input.h>

#include <events.h>
#include <epub/epub.h>
#include <ui/ui.h>

#define POWERCONTROL DT_ALIAS(devicestatecontrol)

LOG_MODULE_REGISTER(main, CONFIG_ZEREADER_LOG_LEVEL);

#if defined(CONFIG_BUTTON_ONOFF)
static const struct device *const longpress_dev = DEVICE_DT_GET(DT_NODELABEL(longpress));
static const struct gpio_dt_spec powercontrol_pin = GPIO_DT_SPEC_GET(POWERCONTROL, gpios);

static void longpress_shutdown_cb(struct input_event *event, void *user_data)
{
	LOG_DBG("Long press detected - turning the device off!");
	epub_free_current_book_resources();
	epub_destroy_book_list();
	gpio_pin_set_dt(&powercontrol_pin, 0);
}
INPUT_CALLBACK_DEFINE(longpress_dev, longpress_shutdown_cb, NULL);

#endif // CONFIG_BUTTON_ONOFF

int main(void)
{
	int error = 0;

	LOG_DBG("ZEReader is starting... %s\n", CONFIG_BOARD_TARGET);

	app_events_init();

#if defined(CONFIG_BUTTON_ONOFF)
	LOG_DBG("Button On/Off configured!");
	if (!gpio_is_ready_dt(&powercontrol_pin))
	{
		LOG_ERR("Device 'powercontrol_pin' is not ready, aborting...");
		return 0;
	}
	error = gpio_pin_configure_dt(&powercontrol_pin, GPIO_OUTPUT_ACTIVE);
	if (error < 0)
	{
		LOG_ERR("Device 'powercontrol_pin' could not be configured as an output!");
		return error;
	}

	if (!device_is_ready(longpress_dev))
	{
		LOG_ERR("Device 'longpress_dev' is not ready, aborting...");
		return 0;
	}
#endif // CONFIG_BUTTON_ONOFF

	context_t context = CONTEXT_READING;

	epub_initialize();

	if (zereader_initialize_peripherals() < 0)
	{
		LOG_ERR("Setup peripherals failed!");
		return 0;
	}

	ui_init(&context);

#if defined(CONFIG_BUTTON_ONOFF)
	error = gpio_pin_set_dt(&powercontrol_pin, 1);
	if (error < 0)
	{
		LOG_ERR("Unable to set 'powercontrol_pin'!");
		return error;
	}
	else
	{
		LOG_DBG("ZEReader is now ON!");
	}
#endif // CONFIG_BUTTON_ONOFF

	zereader_show_logo();
	lv_timer_handler();

	zereader_display_blanking_off();
	zereader_clean_page();

	error = epub_restore_book();
	if (error == 0)
	{
		zereader_print_current_page();
	}

	lv_timer_handler();

	while (1)
	{
		zereader_ui_lock();
		uint32_t sleep_ms = lv_timer_handler();
		zereader_ui_unlock();

		k_msleep(MIN(sleep_ms, INT32_MAX));
	}

	return 0;
}

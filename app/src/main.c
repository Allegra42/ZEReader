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
	gpio_pin_set_dt(&powercontrol_pin, 0);
}
INPUT_CALLBACK_DEFINE(longpress_dev, longpress_shutdown_cb, NULL);

#endif // CONFIG_BUTTON_ONOFF

int main(void)
{
	int error = 0;

	LOG_DBG("ZEReader is starting... %s\n", CONFIG_BOARD_TARGET);

	// Initialize the choosen zephyr,display device
	// and make the device tree description available for the software part.
	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev))
	{
		LOG_ERR("Device 'display_dev' is not ready, aborting...");
		return 0;
	}

	// Make the FIRST ok zephyr,lvgl-button-input node available to the software part
	static const struct device *lvgl_btn_dev;
	lvgl_btn_dev = DEVICE_DT_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_lvgl_button_input));
	if (!device_is_ready(lvgl_btn_dev))
	{
		LOG_ERR("Device 'lvgl_btn_dev' is not ready, aborting...");
		return 0;
	}

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

	context_t context = READING;

	zereader_setup_page();
	zereader_setup_control_buttons(&context);
	epub_initialize();

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
	display_blanking_off(display_dev);
	zereader_clean_page();

	error = epub_restore_book();
	if (error == 0)
	{
		zereader_print_current_page();
	}

	lv_timer_handler();

	while (1)
	{
		uint32_t sleep_ms = lv_timer_handler();

		k_msleep(MIN(sleep_ms, INT32_MAX));
	}

	return 0;
}

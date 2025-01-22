#include <stdio.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>

#include <lvgl.h>
#include <lvgl_input_device.h>

#include "epub/epub.h"
#include "ui/ui.h"

LOG_MODULE_REGISTER(main, CONFIG_ZEREADER_LOG_LEVEL);

int main(void)
{
	LOG_DBG("Hello World - ZEReader! %s\n", CONFIG_BOARD_TARGET);

	context_t context = MENU;

	// Initialize the choosen zephyr,display device
	// -> Make the device tree description available for the software part
	const struct device *display_dev;
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

	zereader_setup_page();
	zereader_setup_control_buttons(&context);

	epub_initialize();

	// Testing
	epub_open_book(0);


	display_blanking_off(display_dev);
	lv_timer_handler();

	while (1)
	{
		uint32_t sleep_ms = lv_timer_handler();

		k_msleep(MIN(sleep_ms, INT32_MAX));
	}

	return 0;
}
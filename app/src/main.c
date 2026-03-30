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

#include <event_handler/event_handler.h>
#include <ui/ui.h>

#define POWERCONTROL DT_ALIAS(devicestatecontrol)

LOG_MODULE_REGISTER(main, CONFIG_ZEREADER_LOG_LEVEL);

#if defined(CONFIG_BUTTON_ONOFF)
static bool shutdown_triggered = false;
static const struct device *const longpress_dev = DEVICE_DT_GET(DT_NODELABEL(longpress));
static const struct gpio_dt_spec powercontrol_pin = GPIO_DT_SPEC_GET(POWERCONTROL, gpios);

static void longpress_shutdown_cb(struct input_event *event, void *user_data)
{
  LOG_DBG("Long press detected - turning the device off!");
  if (!shutdown_triggered)
  {
    shutdown_triggered = true;
    app_event_t app_event = {
        .type = APP_EVENT_SHUTDOWN,
        .data.shutdown.pin = &powercontrol_pin,
    };
    app_post_event(&app_event);
  }
}
INPUT_CALLBACK_DEFINE(longpress_dev, longpress_shutdown_cb, NULL);

#endif // CONFIG_BUTTON_ONOFF

int main(void)
{
  LOG_DBG("ZEReader is starting... %s\n", CONFIG_BOARD_TARGET);

  app_events_init();

#if defined(CONFIG_BUTTON_ONOFF)
  LOG_DBG("Button On/Off configured!");
  if (!gpio_is_ready_dt(&powercontrol_pin))
  {
    LOG_ERR("Device 'powercontrol_pin' is not ready, aborting...");
    return 0;
  }

  if (gpio_pin_configure_dt(&powercontrol_pin, GPIO_OUTPUT_ACTIVE) < 0)
  {
    LOG_ERR("Device 'powercontrol_pin' could not be configured as an output!");
    return -EACCES;
  }

  if (!device_is_ready(longpress_dev))
  {
    LOG_ERR("Device 'longpress_dev' is not ready, aborting...");
    return 0;
  }
#endif // CONFIG_BUTTON_ONOFF

  context_t context = CONTEXT_READING;
  app_event_t event_epub_init = {
    .type = APP_EVENT_EPUB_INIT,
    .data.context.context = &context,
  };
  app_post_event(&event_epub_init);

  if (zereader_initialize_peripherals() < 0)
  {
    LOG_ERR("Setup peripherals failed!");
    return 0;
  }

  ui_init(&context);

#if defined(CONFIG_BUTTON_ONOFF)
  if (gpio_pin_set_dt(&powercontrol_pin, 1) < 0)
  {
    LOG_ERR("Unable to set 'powercontrol_pin'!");
    return -EACCES;
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

  app_event_t event_restore_book = {
    .type = APP_EVENT_RESTORE_BOOK,
    .data.context.context = &context,
  };
  app_post_event(&event_restore_book);

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

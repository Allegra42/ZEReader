/*
 * SPDX-FileCopyrightText: 2026 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#include "device_management/device_management.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(dev_mgmt, CONFIG_ZEREADER_LOG_LEVEL);

const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
const struct adc_dt_spec adc_chan3_vsys = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

int dev_mgmt_init(void)
{
  if (!device_is_ready(display_dev))
  {
    LOG_ERR("Display device not ready");
    return DEV_MGMT_ERROR_DISPLAY_NOT_READY;
  }

  static const struct device *lvgl_btn_dev;
  lvgl_btn_dev = DEVICE_DT_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_lvgl_button_input));
  if (!device_is_ready(lvgl_btn_dev))
  {
    LOG_ERR("LVGL input device not ready");
    return DEV_MGMT_ERROR_LVGL_INPUT_NOT_READY;
  }

  if (!adc_is_ready_dt(&adc_chan3_vsys))
  {
    LOG_ERR("ADC controller device not ready");
    return DEV_MGMT_ERROR_ADC_NOT_READY;
  }

  if (adc_channel_setup_dt(&adc_chan3_vsys) < 0)
  {
    LOG_ERR("Could not setup ADC channel");
    return DEV_MGMT_ERROR_ADC_NOT_READY;
  }

  return DEV_MGMT_SUCCESS;
}

void dev_mgmt_display_blanking_on(void)
{
  display_blanking_on(display_dev);
}

void dev_mgmt_display_blanking_off(void)
{
  display_blanking_off(display_dev);
}

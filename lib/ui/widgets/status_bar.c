/*
 * SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#include <ui/widgets/status_bar.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>

LOG_MODULE_REGISTER(status_bar, CONFIG_ZEREADER_LOG_LEVEL);

#define BATTERY_UPDATE_INTERVAL_MS 120000 // Update every 120 seconds
#define BAT_MIN_MV 3300                   // 0% charge (3.3V)
#define BAT_MAX_MV 4200                   // 100% charge (4.2V)
#define BAT_CHARGER_MV 4900               // Threshold to detect charger (approx 5V)

extern const struct adc_dt_spec adc_chan3_vsys;
static lv_obj_t *bat_label;
static lv_obj_t *chapter_label;

static uint8_t get_battery_percentage(int32_t battery_mv)
{
  if (battery_mv >= BAT_MAX_MV)
    return 100;
  if (battery_mv <= BAT_MIN_MV)
    return 0;

  return (uint8_t)((battery_mv - BAT_MIN_MV) * 100 / (BAT_MAX_MV - BAT_MIN_MV));
}

static const char *get_battery_symbol(uint8_t percentage)
{
  if (percentage > 90)
    return LV_SYMBOL_BATTERY_FULL;
  if (percentage > 70)
    return LV_SYMBOL_BATTERY_3;
  if (percentage > 50)
    return LV_SYMBOL_BATTERY_2;
  if (percentage > 20)
    return LV_SYMBOL_BATTERY_1;

  return LV_SYMBOL_BATTERY_EMPTY;
}

static void update_battery_cb(lv_timer_t *timer)
{
  int err;
  int32_t val_mv;
  uint16_t buf;

  struct adc_sequence sequence = {
      .buffer = &buf,
      /* buffer size in bytes, not number of samples */
      .buffer_size = sizeof(buf),
  };

  (void)adc_sequence_init_dt(&adc_chan3_vsys, &sequence);

  err = adc_read_dt(&adc_chan3_vsys, &sequence);
  if (err < 0)
  {
    LOG_ERR("ADC 3 (VSYS) read failed: (%d)\n", err);
    return;
  }

  val_mv = (int32_t)buf;

  if (adc_raw_to_millivolts_dt(&adc_chan3_vsys, &val_mv) == 0)
  {
    val_mv = (val_mv * 3);
    val_mv = val_mv - 50;
    val_mv /= 10;
    val_mv *= 10;
  }

  if (val_mv >= BAT_CHARGER_MV)
  {
    lv_label_set_text(bat_label, "USB Connected " LV_SYMBOL_CHARGE);
  }
  else
  {
    uint8_t percentage = get_battery_percentage(val_mv);
    lv_label_set_text_fmt(bat_label, "%d%% %s", percentage, get_battery_symbol(percentage));
  }
}

void zereader_status_bar_update_chapter(uint32_t current_chapter, uint32_t num_chapter, char *title)
{
  lv_label_set_text_fmt(chapter_label, "Chapter %d of %d - %s", current_chapter, num_chapter, title);
}

void zereader_status_bar_clear()
{
  lv_label_set_text(chapter_label, "");
}

void zereader_status_bar_create(lv_obj_t *parent)
{
  chapter_label = lv_label_create(parent);
  lv_obj_align(chapter_label, LV_ALIGN_TOP_LEFT, 10, 5);
  lv_label_set_text(chapter_label, ""); // Initial State (empty)

  bat_label = lv_label_create(parent);
  lv_obj_align(bat_label, LV_ALIGN_TOP_RIGHT, -40, 5);
  lv_label_set_text(bat_label, "--% " LV_SYMBOL_BATTERY_EMPTY); // Initial State

  lv_timer_t *bat_timer = lv_timer_create(update_battery_cb, BATTERY_UPDATE_INTERVAL_MS, NULL);
  lv_timer_ready(bat_timer);
}

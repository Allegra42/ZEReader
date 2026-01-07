/*
 * SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#include <event_handler/event_handler.h>
#include <ui/ui.h>
#include <ui/ui_style.h>
#include <ui/widgets/button.h>
#include <ui/widgets/control_bar.h>
#include <ui/widgets/status_bar.h>
#include <ui/widgets/text_area.h>
#include <ui/widgets/logo.h>
#include <ui/widgets/book_menu.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ui, CONFIG_ZEREADER_LOG_LEVEL);

K_MUTEX_DEFINE(lvgl_mutex);

const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

// Currently, only one ADC channel is defined in the device tree.
// Thus, the ADC channel 3, bound to 1/3 VSYS is indexed 0.
// static const struct adc_dt_spec
const struct adc_dt_spec adc_chan3_vsys = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

static lv_obj_t *text_area;
static lv_obj_t *logo;
static lv_obj_t *book_roller;

static zereader_control_bar_t *control_bar;

uint8_t page_ctr;

static void book_roller_event_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = lv_event_get_target_obj(e);
  context_t *context = lv_event_get_user_data(e);

  if (code == LV_EVENT_VALUE_CHANGED)
  {
    uint32_t book_nr = lv_roller_get_selected(obj);

    *context = CONTEXT_READING;
    zereader_control_bar_update_labels(control_bar, *context);
    lv_obj_del(book_roller);
    book_roller = NULL;

    app_event_t event = {
        .type = APP_EVENT_BOOK_SELECTED,
        .data.book_selected.book_id = book_nr,
    };
    app_post_event(&event);
  }
}

void zereader_show_bookmenu(context_t *context, const char *booklist)
{
  book_roller = zereader_book_menu_create(lv_screen_active(), booklist, context, book_roller_event_handler);
}

static void button_1_clicked_cb(lv_event_t *e)
{
  LOG_DBG("Button 1 clicked with event");
  context_t *context = lv_event_get_user_data(e);
  LOG_DBG("Context: %s", context_strings[*context]);
  LOG_DBG("Event code: %d", lv_event_get_code(e));

  if (*context == CONTEXT_READING)
  {
    app_event_t event = {
        .type = APP_EVENT_PREV_PAGE,
    };
    app_post_event(&event);
  }
  else if (*context == CONTEXT_MENU)
  {
    uint32_t type = LV_KEY_UP;
    lv_obj_send_event(book_roller, LV_EVENT_KEY, &type);
  }
}

static void button_2_clicked_cb(lv_event_t *e)
{
  LOG_DBG("Button 2 clicked with event");
  context_t *context = lv_event_get_user_data(e);
  LOG_DBG("Context: %s", context_strings[*context]);

  if (*context == CONTEXT_READING)
  {
    *context = CONTEXT_MENU;
    zereader_control_bar_update_labels(control_bar, *context);
    // zereader_show_bookmenu(context);
    app_event_t event = {
        .type = APP_EVENT_SHOW_BOOKMENU,
        .data.context.context = context,
    };
    app_post_event(&event);
  }
  else if (*context == CONTEXT_MENU)
  {
    uint32_t type = LV_KEY_ENTER;
    lv_obj_send_event(book_roller, LV_EVENT_VALUE_CHANGED, &type);
  }
}

static void button_3_clicked_cb(lv_event_t *e)
{
  LOG_DBG("Button 3 clicked with event");
  context_t *context = lv_event_get_user_data(e);
  LOG_DBG("Context: %s", context_strings[*context]);

  if (*context == CONTEXT_READING)
  {
    // Nothing? Settings?
  }
  else if (*context == CONTEXT_MENU)
  {
    *context = CONTEXT_READING;
    zereader_control_bar_update_labels(control_bar, *context);
    lv_obj_del(book_roller);
    book_roller = NULL;
    // lv_obj_invalidate(text_area);
  }
}

static void button_4_clicked_cb(lv_event_t *e)
{
  LOG_DBG("Button 4 clicked with event");
  context_t *context = lv_event_get_user_data(e);
  LOG_DBG("Context: %s", context_strings[*context]);

  if (*context == CONTEXT_READING)
  {
    app_event_t event = {
        .type = APP_EVENT_NEXT_PAGE,
    };
    app_post_event(&event);
  }
  else if (*context == CONTEXT_MENU)
  {
    uint32_t type = LV_KEY_DOWN;
    lv_obj_send_event(book_roller, LV_EVENT_KEY, &type);
    lv_timer_handler();
  }
}

void zereader_setup_control_buttons(context_t *context)
{
  zereader_control_bar_callbacks_t callbacks = {
      .button_1_cb = button_1_clicked_cb,
      .button_2_cb = button_2_clicked_cb,
      .button_3_cb = button_3_clicked_cb,
      .button_4_cb = button_4_clicked_cb,
  };
  control_bar = zereader_control_bar_create(lv_screen_active(), &style_font_notoserif_14, context, &callbacks);
}

void zereader_setup_statusbar()
{
  zereader_status_bar_create(lv_screen_active());
}

int zereader_initialize_peripherals()
{
  // Initialize the choosen zephyr,display device
  // -> Make the device tree description available for the software part
  if (!device_is_ready(display_dev))
  {
    LOG_ERR("Device not ready, aborting...");
    return UI_ERROR_DISPLAY_NOT_READY;
  }

  // Make the FIRST ok zephyr,lvgl-button-input node available to the software part
  static const struct device *lvgl_btn_dev;
  lvgl_btn_dev = DEVICE_DT_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_lvgl_button_input));
  if (!device_is_ready(lvgl_btn_dev))
  {
    LOG_ERR("Device not ready, aborting...");
    return UI_ERROR_LVGL_INPUT_NOT_READY;
  }

  /* Configure channels individually prior to sampling. */
  if (!adc_is_ready_dt(&adc_chan3_vsys))
  {
    printk("ADC controller device %s not ready\n", adc_chan3_vsys.dev->name);
    return UI_ERROR_ADC_CONTROLLER_NOT_READY;
  }

  if (adc_channel_setup_dt(&adc_chan3_vsys) < 0)
  {
    printk("Could not setup channel #3 (ADC VSYS)\n");
    return UI_ERROR_ADC_CHANNEL_SETUP_FAILED;
  }

  return UI_SUCCESS;
}

void zereader_setup_page()
{
  LOG_DBG("Setup page");

  text_area = zereader_text_area_create(lv_screen_active(), &style_font_notoserif_14);
  page_ctr = 0;
}

void screen_health()
{
  page_ctr++;

  if (page_ctr >= UI_SCREEN_REFRESH_PAGES)
  {
    page_ctr = 0;
    display_blanking_on(display_dev);
    lv_timer_handler();
    display_blanking_off(display_dev);
  }
}

void zereader_print_page(const char *page)
{
  lv_textarea_set_text(text_area, page);
  // lv_textarea_add_text(text_area, epub_get_next_page());
  screen_health();
}

void zereader_clean_page()
{
  zereader_clean_logo();
  lv_textarea_set_text(text_area, " ");
  lv_timer_handler();
  display_blanking_on(display_dev);
  display_blanking_off(display_dev);
}

void zereader_show_logo()
{
  logo = zereader_logo_create(lv_screen_active());
}

void zereader_clean_logo()
{
  if (logo != NULL)
  {
    lv_obj_del(logo);
    logo = NULL;
  }
}

void zereader_show_shutdown_screen()
{
  lv_obj_clean(lv_screen_active());
  lv_timer_handler();
  display_blanking_on(display_dev);
  display_blanking_off(display_dev);
  zereader_show_logo();
}

void zereader_display_blanking_off()
{
  display_blanking_off(display_dev);
}

void zereader_ui_lock(void)
{
  k_mutex_lock(&lvgl_mutex, K_FOREVER);
}

void zereader_ui_unlock(void)
{
  k_mutex_unlock(&lvgl_mutex);
}

void ui_init(context_t *context)
{
  ui_style_init();
  zereader_setup_page();
  zereader_setup_statusbar();
  zereader_setup_control_buttons(context);
}

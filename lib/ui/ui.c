/*
 * SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#include <device_management/device_management.h>
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

void zereader_set_scroll_pos(size_t pos)
{
  lv_obj_update_layout(text_area);
  lv_obj_scroll_to_y(text_area, pos, LV_ANIM_OFF);
}

static void button_1_clicked_cb(lv_event_t *e)
{
  LOG_DBG("Button 1 clicked with event");
  context_t *context = lv_event_get_user_data(e);
  LOG_DBG("Context: %s", context_strings[*context]);
  LOG_DBG("Event code: %d", lv_event_get_code(e));

  if (*context == CONTEXT_READING)
  {
    int val = lv_obj_get_scroll_top(text_area);
    LOG_DBG("lv_obj_get_scroll_top %d", val);
    if (val < 0)
    {
      app_event_t event = {.type = APP_EVENT_PREV_CHAPTER};
      app_post_event(&event);
    }
    else
    {
      app_event_t event = {.type = APP_EVENT_PREV_PAGE};
      app_post_event(&event);
    }
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

    // Save scroll position before opening menu
    app_event_t save_event = {
        .type = APP_EVENT_SAVE_STATE,
        .data.save_state.scroll_pos = lv_obj_get_scroll_y(text_area)};
    app_post_event(&save_event);

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
    int val = lv_obj_get_scroll_bottom(text_area);
    LOG_DBG("lv_obj_get_scroll_bottom %d", val);
    if (val < 0)
    {
      app_event_t event = {.type = APP_EVENT_NEXT_CHAPTER};
      app_post_event(&event);
    }
    else
    {
      app_event_t event = {.type = APP_EVENT_NEXT_PAGE};
      app_post_event(&event);
    }
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
  // Currently, symbols are used instead of text.
  // Thus, it is not needed to set the custom font style -- which does not even include the symbols.
  // control_bar = zereader_control_bar_create(lv_screen_active(), &style_font_notoserif_14, context, &callbacks);
  control_bar = zereader_control_bar_create(lv_screen_active(), NULL, context, &callbacks);
}

void zereader_setup_statusbar()
{
  zereader_status_bar_create(lv_screen_active());
}

void zereader_setup_page()
{
  text_area = zereader_text_area_create(lv_screen_active(), &style_font_notoserif_14);
  page_ctr = 0;
}

void screen_health()
{
  page_ctr++;

  if (page_ctr >= UI_SCREEN_REFRESH_PAGES)
  {
    page_ctr = 0;
    dev_mgmt_display_blanking_on();
    lv_timer_handler();
    dev_mgmt_display_blanking_off();
  }
}

void zereader_print_page(const char *page)
{
  if (page == NULL)
  {
    page = "";
  }
  lv_textarea_set_text(text_area, page);
  lv_obj_update_layout(text_area);
  screen_health();
}

void zereader_recreate_page(const char *content, size_t page)
{
  if (text_area)
  {
    lv_obj_del(text_area);
  }
  zereader_setup_page();
  zereader_print_page(content);
  lv_obj_scroll_to_y(text_area, UI_PAGE_SIZE * page, LV_ANIM_OFF);
}

void zereader_scroll_down()
{
  // - 20 means scroll down 20 px
  lv_obj_scroll_by(text_area, 0, -UI_PAGE_SIZE, LV_ANIM_OFF);
}

void zereader_scroll_up()
{
  // 20 means scroll up 20 px
  lv_obj_scroll_by(text_area, 0, +UI_PAGE_SIZE, LV_ANIM_OFF);
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

void zereader_clean_page()
{
  zereader_clean_logo();
  lv_textarea_set_text(text_area, " ");
  lv_timer_handler();
  dev_mgmt_display_blanking_on();
  dev_mgmt_display_blanking_off();
}

void zereader_show_shutdown_screen()
{
  lv_obj_clean(lv_screen_active());
  lv_timer_handler();
  dev_mgmt_display_blanking_on();
  dev_mgmt_display_blanking_off();
  zereader_show_logo();
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

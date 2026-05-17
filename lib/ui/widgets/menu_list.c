/*
 * SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#include <ui/widgets/menu_list.h>
#include <string.h>

lv_obj_t *zereader_menu_list_create(lv_obj_t *parent, const char *list_str, context_t *context, lv_event_cb_t event_cb)
{
  lv_obj_t *roller = lv_roller_create(parent);

  lv_roller_set_options(roller, list_str, LV_ROLLER_MODE_INFINITE);

  lv_roller_set_visible_row_count(roller, 10);
  lv_obj_set_width(roller, 650);
  lv_obj_center(roller);

  lv_style_selector_t selector = LV_PART_MAIN;
  lv_obj_set_style_anim_time(roller, 0, selector);

  lv_obj_add_event_cb(roller, event_cb, LV_EVENT_ALL, context);

  return roller;
}
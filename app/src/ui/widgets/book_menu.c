/*
 * SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#include "widgets/book_menu.h"
#include <string.h>

lv_obj_t* zereader_book_menu_create(lv_obj_t *parent, const char *book_list_str, context_t *context, lv_event_cb_t event_cb)
{
	lv_obj_t *book_roller = lv_roller_create(parent);

	lv_roller_set_options(book_roller, book_list_str, LV_ROLLER_MODE_INFINITE);

	lv_roller_set_visible_row_count(book_roller, 8);
	lv_obj_center(book_roller);

	lv_style_selector_t selector = LV_PART_MAIN;
	lv_obj_set_style_anim_time(book_roller, 0, selector);

	lv_obj_add_event_cb(book_roller, event_cb, LV_EVENT_ALL, context);

    return book_roller;
}
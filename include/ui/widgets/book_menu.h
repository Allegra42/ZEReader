/*
 * SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef _UI_WIDGET_BOOK_MENU_H_
#define _UI_WIDGET_BOOK_MENU_H_

#include <lvgl.h>
#include <ui/context/context.h>

/**
 * @brief Creates the book menu.
 *
 * @param parent The parent object.
 * @param book_list_str A string containing the list of books, separated by newlines.
 * @param context The UI context.
 * @param event_cb The event callback for the roller.
 * @return A pointer to the created roller object.
 */
lv_obj_t* zereader_book_menu_create(lv_obj_t *parent, const char *book_list_str, context_t *context, lv_event_cb_t event_cb);

#endif /* _UI_WIDGET_BOOK_MENU_H_ */

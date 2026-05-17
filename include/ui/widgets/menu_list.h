/*
 * SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef _UI_WIDGET_MENU_LIST_H_
#define _UI_WIDGET_MENU_LIST_H_

#include <lvgl.h>
#include <ui/context/context.h>

/**
 * @brief Creates a menu list.
 *
 * @param parent The parent object.
 * @param list_str A string containing the list to display, e.g. list of books, separated by newlines.
 * @param context The UI context.
 * @param event_cb The event callback for the roller.
 * @return A pointer to the created roller object.
 */
lv_obj_t *zereader_menu_list_create(lv_obj_t *parent, const char *list_str, context_t *context, lv_event_cb_t event_cb);

#endif /* _UI_WIDGET_MENU_LIST_H_ */

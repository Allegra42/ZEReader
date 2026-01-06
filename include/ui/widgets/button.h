/*
 * SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef _UI_WIDGET_BUTTON_H_
#define _UI_WIDGET_BUTTON_H_

#include <lvgl.h>

/**
 * @brief Creates a new button widget.
 *
 * @param parent The parent object.
 * @param style The style to apply to the button.
 * @param text The text to display on the button.
 * @param align The alignment of the button.
 * @param x_ofs The x offset from the alignment.
 * @param y_ofs The y offset from the alignment.
 * @param event_cb The event callback for the button.
 * @param user_data The user data for the event callback.
 * @return A pointer to the button's label object.
 */
lv_obj_t* zereader_button_create(lv_obj_t *parent, const lv_style_t *style, const char *text, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs, lv_event_cb_t event_cb, void *user_data);

#endif /* _UI_WIDGET_BUTTON_H_ */

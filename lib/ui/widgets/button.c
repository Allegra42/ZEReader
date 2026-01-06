/*
 * SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#include <ui/widgets/button.h>

lv_obj_t* zereader_button_create(lv_obj_t *parent, const lv_style_t *style, const char *text, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs, lv_event_cb_t event_cb, void *user_data)
{
    lv_obj_t *btn = lv_button_create(parent);
    if (style) {
        lv_obj_add_style(btn, (lv_style_t*)style, 0);
    }

    lv_obj_align(btn, align, x_ofs, y_ofs);
    lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, user_data);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);

    return label;
}

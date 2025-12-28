/*
 * SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#include "widgets/text_area.h"

lv_obj_t* zereader_text_area_create(lv_obj_t *parent, const lv_style_t *style)
{
    lv_obj_t *text_area = lv_textarea_create(parent);
    lv_obj_add_style(text_area, (lv_style_t*)style, 0);

    lv_obj_set_x(text_area, 10);
    lv_obj_set_y(text_area, 23);
    lv_obj_set_width(text_area, 780);
    lv_obj_set_height(text_area, 440);

    return text_area;
}

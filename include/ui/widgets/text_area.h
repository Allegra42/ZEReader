/*
 * SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef _UI_WIDGET_TEXT_AREA_H_
#define _UI_WIDGET_TEXT_AREA_H_

#include <lvgl.h>

#define UI_PAGE_SIZE 408

/**
 * @brief Creates the main text area.
 *
 * @param parent The parent object.
 * @param style The style to apply to the text area.
 * @return A pointer to the created text area object.
 */
lv_obj_t* zereader_text_area_create(lv_obj_t *parent, const lv_style_t *style);

#endif /* _UI_WIDGET_TEXT_AREA_H_ */

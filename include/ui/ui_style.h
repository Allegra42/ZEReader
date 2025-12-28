/*
 * SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef _UI_STYLE_H_
#define _UI_STYLE_H_

#include <lvgl.h>

LV_FONT_DECLARE(notoserif_14);

extern lv_style_t style_font_notoserif_14;

void ui_style_init(void);

#endif /* _UI_STYLE_H_ */

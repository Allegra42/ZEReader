/*
 * SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#include <ui/ui_style.h>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wbidi-chars"
#endif /* __GNUC__ */

#include <ui/fonts/notoserif_14.h>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif /* __GNUC__ */

lv_style_t style_font_notoserif_14;

void ui_style_init(void)
{
    lv_style_init(&style_font_notoserif_14);
    lv_style_set_text_font(&style_font_notoserif_14, &notoserif_14);
    lv_style_set_text_line_space(&style_font_notoserif_14, 3); // Enforce a fixed spacing between lines
}

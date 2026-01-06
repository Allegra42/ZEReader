/*
 * SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#include <ui/widgets/control_bar.h>
#include <ui/widgets/button.h>
#include <ui/ui.h>
#include <stdlib.h>

zereader_control_bar_t* zereader_control_bar_create(lv_obj_t *parent, const lv_style_t *style, context_t *context, const zereader_control_bar_callbacks_t *callbacks)
{
    zereader_control_bar_t *control_bar = malloc(sizeof(zereader_control_bar_t));
    if (!control_bar) {
        return NULL;
    }

    control_bar->button_1_label = zereader_button_create(parent, style, BT_PREV, LV_ALIGN_BOTTOM_LEFT, 10, -5, callbacks->button_1_cb, context);
    control_bar->button_2_label = zereader_button_create(parent, style, BT_MENU, LV_ALIGN_BOTTOM_MID, -50, -5, callbacks->button_2_cb, context);
    control_bar->button_3_label = zereader_button_create(parent, style, BT_NONE, LV_ALIGN_BOTTOM_MID, 50, -5, callbacks->button_3_cb, context);
    control_bar->button_4_label = zereader_button_create(parent, style, BT_NEXT, LV_ALIGN_BOTTOM_RIGHT, -10, -5, callbacks->button_4_cb, context);

    return control_bar;
}

void zereader_control_bar_update_labels(zereader_control_bar_t *control_bar, context_t context)
{
    if (context == CONTEXT_READING) {
        lv_label_set_text(control_bar->button_1_label, BT_PREV);
        lv_label_set_text(control_bar->button_2_label, BT_MENU);
        lv_label_set_text(control_bar->button_3_label, BT_NONE);
        lv_label_set_text(control_bar->button_4_label, BT_NEXT);
    } else if (context == CONTEXT_MENU) {
        lv_label_set_text(control_bar->button_1_label, BT_UP);
        lv_label_set_text(control_bar->button_2_label, BT_OK);
        lv_label_set_text(control_bar->button_3_label, BT_EXIT);
        lv_label_set_text(control_bar->button_4_label, BT_DOWN);
    }
}

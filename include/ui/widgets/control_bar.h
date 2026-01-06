/*
 * SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef _UI_WIDGET_CONTROL_BAR_H_
#define _UI_WIDGET_CONTROL_BAR_H_

#include <lvgl.h>
#include <ui/context/context.h>

/**
 * @brief A structure to hold the control bar widgets.
 */
typedef struct {
    lv_obj_t *button_1_label;
    lv_obj_t *button_2_label;
    lv_obj_t *button_3_label;
    lv_obj_t *button_4_label;
} zereader_control_bar_t;

/**
 * @brief A structure to hold the control bar button callbacks.
 */
typedef struct {
    lv_event_cb_t button_1_cb;
    lv_event_cb_t button_2_cb;
    lv_event_cb_t button_3_cb;
    lv_event_cb_t button_4_cb;
} zereader_control_bar_callbacks_t;

/**
 * @brief Creates the control bar with the navigation buttons.
 *
 * @param parent The parent object.
 * @param style The style to apply to the buttons.
 * @param context The UI context.
 * @param callbacks The button callbacks.
 * @return A pointer to the control bar structure.
 */
zereader_control_bar_t* zereader_control_bar_create(lv_obj_t *parent, const lv_style_t *style, context_t *context, const zereader_control_bar_callbacks_t *callbacks);

/**
 * @brief Updates the labels of the control bar buttons based on the context.
 *
 * @param control_bar The control bar object.
 * @param context The current UI context.
 */
void zereader_control_bar_update_labels(zereader_control_bar_t *control_bar, context_t context);

#endif /* _UI_WIDGET_CONTROL_BAR_H_ */

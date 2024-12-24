#pragma once

#include <lvgl.h>
#include <lvgl_input_device.h>

#include "context.h"

extern lv_obj_t *button_left;
extern lv_obj_t *button_mid;
extern lv_obj_t *button_right;

extern lv_obj_t *button_left_label;
extern lv_obj_t *button_mid_label;
extern lv_obj_t *button_right_label;


void zereader_setup_contol_buttons(context_t *context);
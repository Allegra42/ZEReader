/*
 * SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef _UI_WIDGET_LOGO_H_
#define _UI_WIDGET_LOGO_H_

#include <lvgl.h>

/**
 * @brief Creates the logo image.
 *
 * @param parent The parent object.
 * @return A pointer to the created image object.
 */
lv_obj_t* zereader_logo_create(lv_obj_t *parent);

#endif /* _UI_WIDGET_LOGO_H_ */

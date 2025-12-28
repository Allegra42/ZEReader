/*
 * SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#include "widgets/logo.h"
#include <ui/logo/zereaderlogomarx.h>

LV_IMG_DECLARE(zereaderlogomarx);

lv_obj_t* zereader_logo_create(lv_obj_t *parent)
{
    lv_obj_t *logo = lv_image_create(parent);
    lv_image_set_src(logo, &zereaderlogomarx);
    lv_obj_align(logo, LV_ALIGN_CENTER, 0, 30);
    return logo;
}

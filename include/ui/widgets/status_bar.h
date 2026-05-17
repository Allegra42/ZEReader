/*
 * SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef _UI_WIDGET_STATUS_BAR_H_
#define _UI_WIDGET_STATUS_BAR_H_

#include <lvgl.h>

/**
 * @brief Creates the status bar.
 *
 * @param parent The parent object.
 */
void zereader_status_bar_create(lv_obj_t *parent);

/**
 * @brief Updates the chapter status in the status bar.
 *
 * @param current_chapter Number of the current chapter.
 * @param num_chapter Number of chapters in the book.
 *  * @param title Chapter title.
 */
void zereader_status_bar_update_chapter(uint32_t current_chapter, uint32_t num_chapter, char *title);

/**
 * @brief Clears the chapter status in the status bar.
 */
void zereader_status_bar_clear();

#endif /* _UI_WIDGET_STATUS_BAR_H_ */

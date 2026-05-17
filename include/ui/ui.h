/*
 * SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef _UI_H_
#define _UI_H_

#include <lvgl.h>
#include <lvgl_input_device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/display.h>
#include <zephyr/devicetree.h>

#include <ui/context/context.h>

/**
 * @file
 * @brief The ZEReader UI library.
 * @defgroup ui_ui ZEReader UI
 * @ingroup ui
 * @{
 */

#define UI_BOOK_LIST_STR_SIZE 1000
#define UI_SCREEN_REFRESH_PAGES 10

typedef enum
{
  UI_SUCCESS = 0,
  UI_ERROR_DISPLAY_NOT_READY = -1,
  UI_ERROR_LVGL_INPUT_NOT_READY = -2,
  UI_ERROR_ADC_CONTROLLER_NOT_READY = -3,
  UI_ERROR_ADC_CHANNEL_SETUP_FAILED = -4
} ui_error_code_t;

/**
 * @brief Initialize the UI.
 *
 * @param[in] context The UI context.
 */
void ui_init(context_t *context);

/**
 * @brief Clean out the page contents.
 */
void zereader_clean_page();

/**
 * @brief Render a page with the given input.
 *
 * @param[in] page A pointer to the text to render.
 */
void zereader_print_page(const char *page);

/**
 * @brief Recreate the page with new content and scroll position.
 *
 * @param[in] content A pointer to the text to render.
 * @param[in] page The page number to recreate.
 */
void zereader_recreate_page(const char *content, size_t page);

/**
 * @brief Scroll up in the text view.
 *
 */
void zereader_scroll_up(void);

/**
 * @brief Scroll down in the text view.
 *
 */
void zereader_scroll_down(void);

/**
 * @brief Shows the book selection menu.
 *
 * @param[in] context The reader's system context.
 * @param[in] booklist The preformatted book list as needed for the LVGL roller.
 */
void zereader_show_bookmenu(context_t *context, const char *booklist);

/**
 * @brief Shows the configured logo.
 */
void zereader_show_logo();

/**
 * @brief Show a clean shutdown screen.
 */
void zereader_show_shutdown_screen();

/**
 * @brief Updates the chapter status in the status bar.
 *
 * @param current_chapter Number of the current chapter.
 * @param num_chapters Number of chapters in the book.
 * @param title Chapter title.
 */
void zereader_update_chapter_status(uint32_t current_chapter, uint32_t num_chapters, char *title);

/**
 * @brief Shows the chapter selection menu/overview.
 *
 * @param[in] context The reader's system context.
 * @param[in] chapterlist The preformatted chapter list as needed for the LVGL roller.
 */
void zereader_show_chaptermenu(context_t *context, const char *chapterlist);

/**
 * @brief Lock the LVGL mutex.
 */
void zereader_ui_lock(void);

/**
 * @brief Unlock the LVGL mutex.
 */
void zereader_ui_unlock(void);

/** @} */

#endif
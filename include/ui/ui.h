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

#include "context.h"

/**
 * @file
 * @brief The ZEReader UI library.
 * @defgroup ui_ui ZEReader UI
 * @ingroup ui
 * @{
 */

#define BT_OK "ok"
#define BT_EXIT "exit"
#define BT_NEXT "next"
#define BT_PREV "prev"
#define BT_UP "up"
#define BT_DOWN "down"
#define BT_MENU "books"
#define BT_NONE " "

/**
 * @brief Initialize UI related peripherals defined in the device tree.
 *
 * @retval 0 on success.
 * @retval negative on error.
 */
int zereader_initialize_peripherals();

/**
 * @brief Setup and configure the control buttons.
 *
 * @param[in] context The UI context.
 */
void zereader_setup_control_buttons(context_t *context);

/**
 * @brief Setup the page UI elements.
 *
 */
void zereader_setup_page();

/**
 * @brief Setup and configure the status bar.
 */
void zereader_setup_statusbar();

/**
 * @brief Clean out the page contents.
 */
void zereader_clean_page();

/**
 * @brief Show the current page.
 */
void zereader_print_current_page();

/**
 * @brief Show the configured logo.
 */
void zereader_show_logo();

/**
 * @brief Clean out the logo.
 */
void zereader_clean_logo();

/**
 * @brief Turn off display blanking
 */
void zereader_display_blanking_off();

/** @} */

#endif
/*
 * SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef _CONTEXT_H_
#define _CONTEXT_H_

/**
 * @file
 * @brief The UI's context.
 * @defgroup ui_context UI context
 * @ingroup ui
 * @{
 */

/**
 * @brief UI context definition.
 */
typedef enum
{
    MENU = 0, /**< Menu context. */
    READING,  /**< Reading context. */
    SETTINGS  /**< Settings context. */

} context_t;

// When changing the enum, also change the corresponding strings
// in the .c file!
extern const char *context_strings[];

/** @} */

#endif
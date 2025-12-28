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
#define CONTEXT(name) CONTEXT_##name,
#include "ui/context_def.h"
#undef CONTEXT
} context_t;

extern const char *const context_strings[];

/** @} */

#endif
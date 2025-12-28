/*
 * SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#include <ui/context.h>

const char * const context_strings[] = {
#define CONTEXT(name) #name,
#include "ui/context_def.h"
#undef CONTEXT
};
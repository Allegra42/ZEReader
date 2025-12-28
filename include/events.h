/*
 * SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef _EVENTS_H_
#define _EVENTS_H_

#include <stdint.h>

typedef enum {
    APP_EVENT_NONE,
    APP_EVENT_BOOK_SELECTED,
    // Add other events here, e.g.
    // APP_EVENT_NEXT_PAGE,
    // APP_EVENT_PREV_PAGE,
    // APP_EVENT_SHOW_MENU,
} application_event_t;

typedef struct {
    application_event_t type;
    union {
        struct {
            uint32_t book_id;
        } book_selected;
        // Add other event data here
    } data;
} app_event_t;

/**
 * @brief Post an application event.
 *
 * @param event The event to post.
 */
void app_post_event(const app_event_t *event);

/**
 * @brief Initialize the application event system.
 */
void app_events_init(void);

#endif /* _EVENTS_H_ */

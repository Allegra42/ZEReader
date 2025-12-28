/*
 * SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#include <zephyr/kernel.h>
#include <events.h>
#include <epub/epub.h>
#include <ui/ui.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(events, CONFIG_ZEREADER_LOG_LEVEL);

K_MSGQ_DEFINE(app_event_queue, sizeof(app_event_t), 3, 4);

static void handle_book_selected_event(const app_event_t *event)
{
    LOG_DBG("Handling APP_EVENT_BOOK_SELECTED, book_id: %d", event->data.book_selected.book_id);
    book_entry_t *entry = epub_get_book_entry_for_num(event->data.book_selected.book_id);
    epub_open_book(entry);
    zereader_ui_lock();
    zereader_print_next_page();
    zereader_ui_unlock();
    epub_write_current_book_state();
}

// Array of function pointers, indexed by event type
static void (*event_handlers[])(const app_event_t *) = {
    [APP_EVENT_BOOK_SELECTED] = handle_book_selected_event,
};

static void handle_event(const app_event_t *event)
{
    if (event->type < ARRAY_SIZE(event_handlers) && event_handlers[event->type] != NULL) {
        event_handlers[event->type](event);
    } else {
        LOG_WRN("Unhandled event type: %d", event->type);
    }
}

static void event_thread(void)
{
    app_event_t received_event;
    while (1) {
        k_msgq_get(&app_event_queue, &received_event, K_FOREVER);
        handle_event(&received_event);
    }
}

K_THREAD_DEFINE(event_tid, 8192, event_thread, NULL, NULL, NULL, 7, 0, 0);

void app_post_event(const app_event_t *event)
{
    if (k_msgq_put(&app_event_queue, event, K_NO_WAIT) != 0) {
        LOG_WRN("Failed to post event to queue");
    }
}

void app_events_init(void)
{
    // The event thread is started automatically by K_THREAD_DEFINE
    LOG_DBG("Event system initialized");
}

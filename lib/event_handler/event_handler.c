/*
 * SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <event_handler/event_handler.h>
#include <epub/epub.h>
#include <ui/ui.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(events, CONFIG_ZEREADER_LOG_LEVEL);

#define MSQG_MAX_MESSAGES 3
#define EVENT_THREAD_PRIO 7

K_MSGQ_DEFINE(app_event_queue, sizeof(app_event_t), MSQG_MAX_MESSAGES, 4);

static void handle_epub_init_event(const app_event_t *event)
{
  LOG_DBG("Handling APP_EVENT_EPUB_INIT");
  epub_initialize();
}

static void handle_restore_book_event(const app_event_t *event)
{
  int error = epub_restore_book();
  if (error == 0)
  {
    zereader_ui_lock();
    const char *chapter_content = epub_get_current_chapter_content();
    if (chapter_content)
    {
      zereader_recreate_page(chapter_content, epub_get_page());
    }
    zereader_ui_unlock();
  }
}

static void handle_save_state_event(const app_event_t *event)
{
  LOG_DBG("Handling APP_EVENT_SAVE_STATE");
  epub_write_current_book_state();
}

static void handle_book_selected_event(const app_event_t *event)
{
  LOG_DBG("Handling APP_EVENT_BOOK_SELECTED, book_id: %d", event->data.book_selected.book_id);
  book_entry_t *entry = epub_get_book_entry_for_num(event->data.book_selected.book_id);
  LOG_DBG("Got epub_get_book_entry_for_num");
  epub_open_book(entry);
  LOG_DBG("epub_open()");
  zereader_ui_lock();
  const char *chapter_content = epub_get_current_chapter_content();
  if (chapter_content)
  {
    zereader_recreate_page(chapter_content, 0);
  }
  zereader_ui_unlock();
  epub_write_current_book_state();
}

static void handle_prev_chapter_event(const app_event_t *event)
{
  LOG_DBG("Handling APP_EVENT_PREV_CHAPTER");
  if (epub_get_prev_chapter() == 0)
  {
    zereader_ui_lock();
    const char *chapter_content = epub_get_current_chapter_content();
    if (chapter_content)
    {
      zereader_print_page(chapter_content);
    }
    zereader_ui_unlock();
    epub_write_current_book_state();
  }
}

static void handle_next_chapter_event(const app_event_t *event)
{
  LOG_DBG("Handling APP_EVENT_NEXT_CHAPTER");
  if (epub_get_next_chapter() == 0)
  {
    zereader_ui_lock();
    const char *chapter_content = epub_get_current_chapter_content();
    if (chapter_content)
    {
      zereader_recreate_page(chapter_content, 0);
    }
    zereader_ui_unlock();
    epub_write_current_book_state();
  }
}

static void handle_next_page_event(const app_event_t *event)
{
  LOG_DBG("Handling APP_EVENT_NEXT_PAGE");
  zereader_ui_lock();
  zereader_scroll_down();
  zereader_ui_unlock();
  epub_update_page(1);
  epub_write_current_book_state();
}

static void handle_prev_page_event(const app_event_t *event)
{
  LOG_DBG("Handling APP_EVENT_PREV_PAGE");
  zereader_ui_lock();
  zereader_scroll_up();
  zereader_ui_unlock();
  epub_update_page(-1);
  epub_write_current_book_state();
}

static void handle_show_bookmenu_event(const app_event_t *event)
{
  LOG_DBG("Handling APP_EVENT_SHOW_BOOKMENU");
  char book_list_str[UI_BOOK_LIST_STR_SIZE];
  memset(book_list_str, 0, sizeof(book_list_str));
  size_t remaining_len = sizeof(book_list_str);
  char *current_pos = book_list_str;

  book_list_t *books = epub_get_book_list();

  while (books != NULL)
  {
    int written = snprintf(current_pos, remaining_len, "%zu - %s - %s", books->book->number, books->book->author, books->book->title);
    if (written > 0 && written < remaining_len)
    {
      current_pos += written;
      remaining_len -= written;
    }

    if (books->next != NULL && remaining_len > 1)
    {
      *current_pos = '\n';
      current_pos++;
      remaining_len--;
    }
    books = books->next;
  }

  zereader_ui_lock();
  zereader_show_bookmenu(event->data.context.context, book_list_str);
  zereader_ui_unlock();
}

static void handle_shutdown_event(const app_event_t *event)
{
  LOG_DBG("Handling APP_EVENT_SHUTDOWN");
  zereader_ui_lock();
  zereader_show_shutdown_screen();
  epub_free_current_book_resources();
  epub_destroy_book_list();
  zereader_ui_unlock();
  // Wait until the logo is shown before turning off the device!
  k_msleep(2000);
  gpio_pin_set_dt(event->data.shutdown.pin, 0);
}

// Array of function pointers, indexed by event type
static void (*event_handlers[])(const app_event_t *) = {
    [APP_EVENT_EPUB_INIT] = handle_epub_init_event,
    [APP_EVENT_RESTORE_BOOK] = handle_restore_book_event,
    [APP_EVENT_BOOK_SELECTED] = handle_book_selected_event,
    [APP_EVENT_PREV_CHAPTER] = handle_prev_chapter_event,
    [APP_EVENT_NEXT_CHAPTER] = handle_next_chapter_event,
    [APP_EVENT_NEXT_PAGE] = handle_next_page_event,
    [APP_EVENT_PREV_PAGE] = handle_prev_page_event,
    [APP_EVENT_SHOW_BOOKMENU] = handle_show_bookmenu_event,
    [APP_EVENT_SAVE_STATE] = handle_save_state_event,
    [APP_EVENT_SHUTDOWN] = handle_shutdown_event,
};

static void handle_event(const app_event_t *event)
{
  if (event->type < ARRAY_SIZE(event_handlers) && event_handlers[event->type] != NULL)
  {
    event_handlers[event->type](event);
  }
  else
  {
    LOG_WRN("Unhandled event type: %d", event->type);
  }
}

static void event_thread(void)
{
  app_event_t received_event;
  while (1)
  {
    k_msgq_get(&app_event_queue, &received_event, K_FOREVER);
    handle_event(&received_event);
  }
}

K_THREAD_DEFINE(event_tid, 8192, event_thread, NULL, NULL, NULL, EVENT_THREAD_PRIO, 0, 0);

void app_post_event(const app_event_t *event)
{
  if (k_msgq_put(&app_event_queue, event, K_NO_WAIT) != 0)
  {
    LOG_WRN("Failed to post event to queue");
  }
}

void app_events_init(void)
{
  // The event thread is started automatically by K_THREAD_DEFINE
  LOG_DBG("Event system initialized");
}

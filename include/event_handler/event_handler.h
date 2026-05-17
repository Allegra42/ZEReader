/*
 * SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef _EVENT_HANDLER_H_
#define _EVENT_HANDLER_H_

#include <stdint.h>
#include <zephyr/drivers/gpio.h>
#include <ui/context/context.h>

/**
 * @file
 * @brief The ZEReader Event Handler.
 * @defgroup event_handler Event-based system execution flow
 * @ingroup event
 * @{
 */

/**
 * @brief Represents possible events within the ZEReader application.
 */
typedef enum
{
  /**
   * No event.
   */
  APP_EVENT_NONE,
  /**
   * EPUB init event.
   */
  APP_EVENT_EPUB_INIT,
  /**
   * Restore book event.
   */
  APP_EVENT_RESTORE_BOOK,
  /**
   * Book selected event.
   */
  APP_EVENT_BOOK_SELECTED,
  /**
   * Chapter selected event.
   */
  APP_EVENT_CHAPTER_SELECTED,
  /**
   * Prev chapter event.
   */
  APP_EVENT_PREV_CHAPTER,
  /**
   * Next chapter event.
   */
  APP_EVENT_NEXT_CHAPTER,
  /**
   * Prev page event.
   */
  APP_EVENT_PREV_PAGE,
  /**
   * Next page event.
   */
  APP_EVENT_NEXT_PAGE,
  /**
   * Show book menu event.
   */
  APP_EVENT_SHOW_BOOKMENU,
  /**
   * Show chapter menu event.
   */
  APP_EVENT_SHOW_CHAPTERMENU,
  /**
   * Save reading state event.
   */
  APP_EVENT_SAVE_STATE,
  /**
   * Shutdown event.
   */
  APP_EVENT_SHUTDOWN,
} application_event_t;

/**
 * @brief Represents an application event.
 */
typedef struct
{
  /**
   * The event type.
   */
  application_event_t type;
  /**
   * The event data (optional fields).
   */
  union
  {
    /**
     * Transports the ID of a selected element.
     */
    struct
    {
      uint32_t id;
    } selected;
    /**
     * Transports a state context.
     */
    struct
    {
      context_t *context;
    } context;
    /**
     * Transports a GPIO for shutting down the device.
     */
    struct
    {
      const struct gpio_dt_spec *pin;
    } shutdown;
    /**
     * Transports the current scroll position for saving the reading state.
     */
    struct
    {
      size_t scroll_pos;
    } save_state;
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
 *
 * The event thread is started automatically by a K_THREAD_DEFINE.
 * The app_events_init function is for logging purposes only.
 */
void app_events_init(void);

/** @} */
#endif /* _EVENT_HANDLER_H_ */

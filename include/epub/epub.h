/*
 * SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef _EPUB_H_
#define _EPUB_H_

#include <stdint.h>
#include <stddef.h>

/**
 * @file
 * @brief The ZEReader EPUB library.
 * @defgroup epub_management EPUB management and parsing
 * @ingroup epub
 * @{
 */

/**
 * @brief The maximum file name length supported by the FatFS filesystem is 255.
 */
#define EPUB_FILE_LEN_MAX 255

/**
 * @brief The maximum buffer size for the state string.
 */
#define EPUB_STATE_STRING_SIZE 500

/**
 * @brief The read size for the container.xml file.
 */
#define EPUB_CONTAINER_XML_READ_SIZE 350

/**
 * @brief The read size for the content.opf file.
 */
#define EPUB_OPF_READ_SIZE 800

/**
 * @brief The maximum buffer size for listing directories.
 *
 * Needs a fixed size.
 */
#define EPUB_LSDIR_CHARS_MAX 4096

/**
 * @brief The maximum buffer size for parsing chapter files.
 */
#define EPUB_PARSE_BUFFER_SIZE 4096 // Increased to accommodate larger raw content for parsing

/**
 * @brief The maximum configured page read size.
 *
 * This defines the maximum internal buffer size for a page. The UI will request
 * a specific size, but it cannot exceed this internal buffer size.
 */
#define EPUB_MAX_PAGE_BUFFER_SIZE 1500 // A reasonable maximum for a display page

/**
 * @brief The path and name for the reading state file.
 *
 * Currently a single global state is used.
 */
#define STATE_FILE "/SD:/.statefile"

/**
 * @brief Represents one EPUB E-Book.
 *
 */
typedef struct
{
  /**
   * The book's internal number.
   */
  size_t number;

  /**
   * The book title.
   */
  char *title;

  /**
   * The book author.
   */
  char *author;

  /**
   * The path of the book's root file.
   * Most often OEBPS/content.opf.
   */
  char *entry_point;

  /**
   * The book's base content directory path.
   */
  char *content_dir;
} book_entry_t;

/**
 * @brief A linked list to store all book representations.
 */
typedef struct llist
{
  /**
   * The current book entry.
   */
  book_entry_t *book;

  /**
   * A pointer to the next book entry.
   */
  struct llist *next;
} book_list_t;

/**
 * @brief Represents one chapter in a book.
 */
typedef struct
{
  /**
   * The number of the chapter inside the book.
   */
  size_t number;

  /**
   * The chapter's file path.
   */
  char *path;
} chapter_entry_t;

/**
 * @brief A double-linked list to represent the chapters in a book.
 */
typedef struct dllist
{
  /**
   * The current chapter entry.
   */
  chapter_entry_t *chapter;

  /**
   * A pointer to the next chapter entry.
   */
  struct dllist *next;

  /**
   * A pointer to the previous chapter entry.
   */
  struct dllist *prev;
} chapter_list_t;

/**
 * @brief Represents the current book read.
 */
typedef struct
{
  /**
   * The number of the book's chapters.
   */
  uint16_t num_chapters;

  /**
   * A pointer to the book's chapter list.
   */
  chapter_list_t *chapter_list;

  /**
   * A pointer to the currently active chapter.
   */
  chapter_list_t *current_chapter;

  /**
   * The currently read raw page chunk.
   */
  char page[EPUB_MAX_PAGE_BUFFER_SIZE];

  /**
   * The current chapter's filename.
   */
  char *chapter_filename;

  /**
   * The raw content of the current chapter file.
   */
  char *chapter_raw_content;

  /**
   * The size of the raw content of the current chapter file.
   */
  size_t chapter_raw_content_size;

  /**
   * The prettified content of the current chapter file.
   */
  char *chapter_prettified_content;

  /**
   * The size of the prettified content of the current chapter file.
   */
  size_t chapter_prettified_content_size;

  /**
   * The current book's content root directory path.
   */
  char *content_dir;

  /**
   * The current book's state.
   */
  struct
  {
    /**
     * The currently read book's title.
     *
     * Use to find and open the right book at rebooting the Reader.
     */
    // May use title instead of number in future
    char *title;

    /**
     * The currently read chapter.
     */
    int32_t chapter;

    /**
     * The page within a chapter.
     */
    int32_t page;

  } state;

} current_book_t;

/**
 * @brief Free all dynamically allocated resources within the book list.
 *
 * @retval 0 on success.
 */
int epub_destroy_book_list();

/**
 * @brief Initialize the SD card and fetch EPUBs, authors and titles.
 *
 * @retval 0 on success.
 */
int epub_initialize();

/**
 * @brief Get the book list.
 *
 * @retval book_list_t a linked list to store the books.
 */
book_list_t *epub_get_book_list();

/**
 * @brief Get the @c book_entry_t representation for a given number in the book list.
 *
 * @param[in] number The number of the book within the book_list.
 *
 * @retval book_entry_t the book_entry representation on success.
 * @retval NULL if there is not book_entry for the given number.
 */
book_entry_t *epub_get_book_entry_for_num(uint16_t number);

/**
 * @brief Get the @c book_entry_t representation for a given book title.
 *
 * @param[in] title The title of the book.
 *
 * @retval book_entry_t the book_entry representation on success.
 * @retval NULL if there is no book_entry for the given title.
 */
book_entry_t *epub_get_book_entry_for_title(char *title);

/**
 * @brief Load and open the EPUB represented by the given @c book_entry_t entry.
 *
 * @param[in] book The book's @c book_entry_t representation.
 *
 * @retval 0 on success.
 */
int epub_open_book(book_entry_t *book);

/**
 * @brief Restore a book's reading state from a saved state file.
 *
 * @retval 0 on success.
 */
int epub_restore_book();

/**
 * @brief Get the full content of the current prettified chapter.
 *
 * @returns A pointer to the chapter content string. Returns NULL if no book/chapter is loaded.
 */
const char *epub_get_current_chapter_content(void);

/**
 * @brief Go to the next chapter.
 *
 * @retval 0 on success.
 */
int epub_get_next_chapter();

/**
 * @brief Go to the previous chapter.
 *
 * @retval 0 on success.
 */
int epub_get_prev_chapter();

/**
 * @brief Update the current book's chapter page counter.
 *
 * @param[in] update The page difference, e.g. +1 or -1.
 */
void epub_update_page(int32_t update);

/**
 * @brief Get the current book's page within a chapter.
 *
 * @retval A positive page number or 0 on success.
 */
int32_t epub_get_page(void);

/**
 * @brief Save the current book's state in a state file on the inserted SD card.
 *
 * @retval >=0 written bytes, on success.
 *
 */
int epub_write_current_book_state();

/**
 * @brief Restore the book state from a state file on the inserted SD card.
 *
 * @returns A freshly allocated current book
 */
current_book_t *epub_get_current_book_state();

/**
 * @brief Free all dynamically allocated resources within the current book.
 *
 * @retval 0 on success.
 */
int epub_free_current_book_resources();

/** @} */
#endif

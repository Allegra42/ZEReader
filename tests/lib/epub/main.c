/**
 * @file test epub library
 * 
 * Testsuit for the epub library
 */

 #include <limits.h>
 #include <zephyr/ztest.h>

 #include <lib/epub/epub.h>

 book_list_t *book_list = NULL;
 current_book_t *current_book_t = NULL;

 ZTEST(epub_lib, test_epub_add_book_entry)
 {
    free(current_book);
    current_book = (current_book_t *)malloc(sizeof(current_book_t));

    zassert_is_null(epub_get_chapter_entry(0));
 }

 ZTEST_SUITE(epub_lib, NULL, NULL, NULL, NULL, NULL);
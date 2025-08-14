/*
 * SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <epub/epub.h>
#include <sd/sd.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(epub, CONFIG_ZEREADER_LOG_LEVEL);

book_list_t *book_list = NULL;
current_book_t *current_book = NULL;

char *strdup(const char *s)
{
    size_t len = strlen(s) + 1;
    void *new = (char *)malloc(len);
    if (new == NULL)
        return NULL;
    return (char *)memcpy(new, s, len);
}

char *strndup(const char *s, size_t n)
{
    size_t len = strnlen(s, n);
    char *new = (char *)malloc(len + 1);
    if (new == NULL)
        return NULL;
    new[len] = '\0';
    return (char *)memcpy(new, s, len);
}

book_entry_t *epub_add_book_entry()
{
    book_list_t *node = (book_list_t *)malloc(sizeof(book_list_t));
    node->book = (book_entry_t *)malloc(sizeof(book_entry_t));
    node->next = NULL;

    if (book_list == NULL)
    {
        book_list = node;
        return node->book;
    }

    book_list_t *current = book_list;
    while (current->next != NULL)
    {
        current = current->next;
    }

    current->next = node;
    return node->book;
}

book_entry_t *epub_get_book_entry_for_num(uint16_t number)
{
    book_list_t *current = book_list;
    while (current != NULL)
    {
        if (current->book->number == number)
        {
            return current->book;
        }
        current = current->next;
    }
    return NULL;
}

book_entry_t *epub_get_book_entry_for_title(char *title)
{
    book_list_t *current = book_list;
    while (current != NULL)
    {
        if (strcmp(current->book->title, title) == 0)
        {
            return current->book;
        }
        current = current->next;
    }
    return NULL;
}

book_list_t *epub_get_book_list()
{
    return book_list;
}

int epub_write_current_book_state()
{
    LOG_DBG("Writing the book's state");

    size_t to_write;
    char state_string[500];

    sprintf(state_string, "%s\n%d\n%d\n", current_book->state.title, current_book->state.chapter, current_book->state.file_offset);
    to_write = strlen(state_string);

    int ret = sd_write_chunk(STATE_FILE, state_string, &to_write);
    if (ret)
    {
        LOG_ERR("Writing the book's state file failed!");
    }
    return ret;
}

void epub_get_current_book_state()
{
    size_t size = 500;
    char buf[size];
    size_t offset = 0;

    char *title = 0;
    size_t chapter = 0;
    size_t ofs = 0;

    sd_read_chunk(STATE_FILE, &offset, buf, &size);
    LOG_DBG("Current state:\n %s", buf);

    char *token = strtok(buf, "\n");
    if (token != NULL)
    {
        title = strdup(token);
        LOG_DBG("Title: %s", title);
    }

    token = strtok(NULL, "\n");
    if (token != NULL)
    {
        chapter = strtoul(token, NULL, 10);
        LOG_DBG("Chapter: %d", chapter);
    }

    token = strtok(NULL, "\n");
    if (token != NULL)
    {
        ofs = strtoul(token, NULL, 10);
        LOG_DBG("Offset: %d", ofs);
    }

    free(current_book);
    current_book = (current_book_t *)malloc(sizeof(current_book_t));
    current_book->state.title = strdup(title);
    current_book->state.chapter = chapter;
    current_book->state.file_offset = ofs;
}

chapter_entry_t *epub_add_chapter_entry()
{
    chapter_list_t *node = (chapter_list_t *)malloc(sizeof(chapter_list_t));
    node->chapter = (chapter_entry_t *)malloc(sizeof(chapter_entry_t));
    node->next = NULL;
    node->prev = NULL;

    if (current_book->chapter_list == NULL)
    {
        current_book->chapter_list = node;
        return node->chapter;
    }

    chapter_list_t *current = current_book->chapter_list;
    while (current->next != NULL)
    {
        current = current->next;
    }

    current->next = node;
    current->next->prev = current;
    return node->chapter;
}

chapter_entry_t *epub_get_chapter_entry(uint16_t number)
{
    chapter_list_t *current = current_book->chapter_list;
    while (current != NULL)
    {
        if (current->chapter->number == number)
        {
            return current->chapter;
        }
        current = current->next;
    }
    return NULL;
}

chapter_list_t *epub_get_chapter_list()
{
    return current_book->chapter_list;
}

char *epub_content_opf_metadata_get_element(const char *search_tag, const char *filename,
                                            size_t file_read_size)
{
    char *delim = ">";
    char read_buffer[file_read_size];
    uint16_t len_search_tag = strlen(search_tag);

    size_t offset = 0;
    sd_read_chunk(filename, &offset, read_buffer, &file_read_size);

    char *token = strtok(read_buffer, delim);
    while (token)
    {
        if (strstr(token, search_tag) != 0)
        {
            token[strlen(token) - len_search_tag] = 0;
            return token;
        }

        token = strtok(NULL, delim);
    }
    return NULL;
}

char *epub_container_xml_get_rootpath(const char *folder, const char *filepath)
{
    size_t read_size = 350;
    char *delim = " ";
    char buffer[read_size];

    size_t offset = 0;
    sd_read_chunk(filepath, &offset, buffer, &read_size);

    char *token = strtok(buffer, delim);
    while (token)
    {
        if (strstr(token, "full-path") != 0)
        {
            LOG_DBG("Token: %s", token);

            char *rootpath = (char *)malloc(5 + strlen(token) - 12 + strlen(folder) + 1);
            memcpy(rootpath, "/SD:/", 5);
            strncat(rootpath, folder, strlen(folder));
            strcat(rootpath, "/");
            strncat(rootpath, token + 11, strlen(token) - 12);

            return rootpath;
        }
        token = strtok(NULL, delim);
    }
    return NULL;
}

int epub_get_epub_rootfiles()
{
    int ret;
    int found = 0;

    char *container_xml = "/META-INF/container.xml";
    char *book_path;
    char *rootpath;

    struct fs_dir_t dir_obj;
    static struct fs_dirent entry;

    fs_dir_t_init(&dir_obj);

    ret = fs_opendir(&dir_obj, "/SD:/");
    if (ret)
    {
        LOG_ERR("Open root directory failed!");
        return ret;
    }

    while (true)
    {
        ret = fs_readdir(&dir_obj, &entry);
        if (ret)
        {
            LOG_DBG("Could not read directory");
            return ret;
        }

        if (entry.name[0] == 0)
        {
            break;
        }

        if (entry.type == FS_DIR_ENTRY_DIR)
        {
            struct fs_file_t f_obj;

            // Found folder, test if META-INF/container.xml exists
            book_path = sd_build_full_path(entry.name, container_xml, &ret);

            ret = sd_open(book_path, &f_obj);
            if (ret == 0)
            {
                LOG_DBG("Found valid book %s", book_path);

                rootpath = epub_container_xml_get_rootpath(entry.name, book_path);
                if (rootpath != NULL)
                {
                    LOG_DBG("Found rootfile path: %s", rootpath);

                    book_entry_t *book = epub_add_book_entry();
                    book->number = found;
                    book->entry_point = strdup(rootpath);
                    book->root_dir = strdup(entry.name);

                    sd_close(&f_obj);

                    found++;
                }
            }
        }

        LOG_DBG("[%s] %s", entry.type == FS_DIR_ENTRY_DIR ? "DIR " : "FILE", entry.name);
    }

    ret = fs_closedir(&dir_obj);
    if (ret)
    {
        LOG_ERR("Could not close root directory");
        return ret;
    }

    return 0;
}

void epub_get_authors_and_titles()
{
    size_t file_read_size = 800;
    const char *search_creator = "</dc:creator";
    const char *search_title = "</dc:title";
    char *element;

    book_list_t *current_elem = book_list;
    while (current_elem != NULL)
    {
        book_entry_t *book = current_elem->book;
        element = epub_content_opf_metadata_get_element(search_creator, book->entry_point, file_read_size);
        book->author = strdup(element);

        element = epub_content_opf_metadata_get_element(search_title, book->entry_point, file_read_size);
        book->title = strdup(element);
        current_elem = current_elem->next;
    }
}

int char_get_index(char *str, char c)
{
    char *end = strchr(str, c);
    if (end == NULL)
    {
        return -1;
    }
    return (int)(end - str);
}

int epub_parse_chapter_files(const char *content_opf)
{
    int ret = 0;
    size_t offset = 0;
    size_t read_size = 800;
    char buf[read_size];
    char *delim = ">";
    char *search_tag = "href=\"";
    uint16_t len_search_tag = strlen(search_tag);

    while (read_size == 800)
    {
        ret = sd_read_chunk(content_opf, &offset, buf, &read_size);
        if (ret)
        {
            LOG_ERR("Could parse chapters!");
            return ret;
        }

        char *token = strtok(buf, delim);
        while (token)
        {
            char *found_href = strstr(token, search_tag);
            if (found_href != 0)
            {
                // href="Text/BD0B5209B23149EC818E6FE711A195F6.xhtml" id="x978-3-641-16998-5-36" media-type="application/xhtml+xml"
                // By adding the length of href=", we get our start pointer
                char *path = found_href + len_search_tag;

                // The next occurrence of " is the end of the path
                int path_len = char_get_index(path, '"');

                if (path_len >= EPUB_FILE_LEN_MAX)
                {
                    LOG_ERR("Found path is too long");
                    return -1;
                }
                else if (path_len < EPUB_FILE_LEN_MAX && path_len > 0)
                {
                    chapter_entry_t *chapter = epub_add_chapter_entry();
                    chapter->path = (char *)malloc((5 + sizeof(current_book->root_dir) + 7 + path_len) * sizeof(char));
                    strcpy(chapter->path, "/SD:/");
                    strcat(chapter->path, current_book->root_dir);
                    strcat(chapter->path, "/OEBPS/");
                    strncat(chapter->path, path, path_len);
                    chapter->number = current_book->num_chapters;

                    LOG_DBG("chapter path: %s", chapter->path);
                    current_book->num_chapters++;
                }
                else if (path_len < 0)
                {
                    // Path len is negative, current read batch is finished
                    // We need to move the offset back to the start of this href to make sure we find it
                    offset -= strlen(token);
                }
            }

            token = strtok(NULL, delim);
        }
    }
    return ret;
}

bool is_html(char *path)
{
    return strstr(path, "html");
}

int epub_prettify_page()
{
    char *delim = ">\n";

    char *token = strtok(current_book->page, delim);
    while (token)
    {
        // Handle <p *> </p>
        if (strstr(token, "</p") != 0)
        {
            strncat(current_book->pretty_page, token, strlen(token) - 3);
            strcat(current_book->pretty_page, "\n");
        }

        if (strstr(token, "</div") != 0)
        {
            strncat(current_book->pretty_page, token, strlen(token) - 5);
            strcat(current_book->pretty_page, "\n");
        }

        if (strstr(token, "</span") != 0)
        {
            strncat(current_book->pretty_page, token, strlen(token) - 6);
        }

        if (strstr(token, "</em") != 0)
        {
            strncat(current_book->pretty_page, token, strlen(token) - 4);
        }

        // Not meaningful for the most books
        // if (strstr(token, "</title") != 0)
        // {
        //     strncat(current_book->pretty_page, token, strlen(token) - 7);
        // }

        if (strstr(token, "</a") != 0)
        {
            strncat(current_book->pretty_page, token, strlen(token) - 3);
        }

        if (strstr(token, "</i") != 0)
        {
            strncat(current_book->pretty_page, token, strlen(token) - 3);
        }

        if (strstr(token, "<br/") != 0)
        {
            strcat(current_book->pretty_page, "\n");
        }

        if (strstr(token, "</li") != 0)
        {
            strcat(current_book->pretty_page, "\n");
        }

        // h1, h2, ...
        // Just using </h does not work as </head would also be found and result in strange artifacts
        if (strstr(token, "</h1") != 0)
        {
            if (strlen(token) > 4)
            {
                strncat(current_book->pretty_page, token, strlen(token) - 4);
            }

            strcat(current_book->pretty_page, "\n\n");
        }

        if (strstr(token, "</h2") != 0)
        {
            if (strlen(token) > 4)
            {
                strncat(current_book->pretty_page, token, strlen(token) - 4);
            }

            strcat(current_book->pretty_page, "\n\n");
        }

        token = strtok(NULL, delim);
    }

    return 0;
}

int epub_get_next_chapter()
{
    current_book->state.file_offset = 0;
    LOG_DBG("Get next chapter");
    if (current_book->state.chapter == current_book->num_chapters - 1)
    {
        LOG_DBG("Book finished!");
        return 0;
    }
    if (current_book->current_chapter == NULL)
    {
        // Set linked list pointer to the first element
        current_book->current_chapter = current_book->chapter_list;
    }

    if (current_book->current_chapter->next != NULL)
    {
        current_book->current_chapter = current_book->current_chapter->next;
        current_book->state.chapter++;

        if (!is_html(current_book->current_chapter->chapter->path))
        {
            epub_get_next_chapter();
        }
    }
    return 0;
}

int epub_get_chapter(size_t index)
{
    LOG_DBG("Get chapter");
    if (current_book->state.chapter >= current_book->num_chapters - 1)
    {
        LOG_DBG("Book finished!");
        return 0;
    }
    if (current_book->current_chapter == NULL)
    {
        // Set linked list pointer to the first element
        current_book->current_chapter = current_book->chapter_list;
    }

    if (index == 0)
    {
        return 0;
    }
    else
    {
        current_book->current_chapter = current_book->current_chapter->next;
        // current_book->state.chapter++;
        return epub_get_chapter(index - 1);
    }
    return 0;
}

int epub_get_prev_chapter()
{
    LOG_DBG("Get prev chapter");
    if (current_book->state.chapter == 0)
    {
        LOG_DBG("Already at the beginning");
        return 0;
    }

    if (current_book->current_chapter->prev != NULL)
    {
        current_book->current_chapter = current_book->current_chapter->prev;
        current_book->state.chapter--;

        if (!is_html(current_book->current_chapter->chapter->path))
        {
            epub_get_prev_chapter();
        }
        else
        {
            LOG_DBG("Opening previous file %s", current_book->current_chapter->chapter->path);
            sd_tell_end_offset(current_book->current_chapter->chapter->path, &current_book->state.file_offset);
            LOG_DBG("End offset of the prev chapter: %d", current_book->state.file_offset);
        }
    }
    return 0;
}

int epub_fetch_next_page_chunk()
{
    size_t read_size = (EPUB_PAGE_SIZE - 1);
    LOG_DBG("Current offset: %d", current_book->state.file_offset);

    int ret = sd_read_chunk(current_book->current_chapter->chapter->path, &current_book->state.file_offset,
                            current_book->page, &read_size);
    current_book->page[read_size] = 0;

    LOG_DBG("%s", current_book->page);

    if (read_size < (EPUB_PAGE_SIZE - 1))
    {
        LOG_DBG("Open next chapter");
        epub_get_next_chapter();
    }

    return ret;
}

char *epub_get_next_page()
{

    if (epub_fetch_next_page_chunk() == 0)
    {
        memset(current_book->pretty_page, 0, sizeof(current_book->pretty_page));
        epub_prettify_page();
        LOG_DBG("Pretty page: %s", current_book->pretty_page);
        return current_book->pretty_page;
    }
    LOG_DBG("Can't get page");
    return "";
}

int epub_fetch_prev_page_chunk()
{
    size_t read_size = (EPUB_PAGE_SIZE - 1);

    if (current_book->state.file_offset <= read_size || current_book->state.file_offset < 0)
    {
        LOG_DBG("Open previous chapter");
        epub_get_prev_chapter();
        if (current_book->state.file_offset > read_size)
        {
            size_t end_offset = current_book->state.file_offset % read_size;
            current_book->state.file_offset -= end_offset;
        }
        else
        {
            current_book->state.file_offset = 0;
        }
    }
    else if (current_book->state.file_offset <= (2 * read_size))
    {
        current_book->state.file_offset = 0;
    }
    else if (current_book->state.file_offset > (2 * read_size))
    {
        current_book->state.file_offset -= 2 * read_size;
    }

    LOG_DBG("Opening prev page at offset :%d", current_book->state.file_offset);

    int ret = sd_read_chunk(current_book->current_chapter->chapter->path, &current_book->state.file_offset,
                            current_book->page, &read_size);
    current_book->page[read_size] = 0;
    LOG_DBG("%s", current_book->page);

    return ret;
}

char *epub_get_prev_page()
{

    if (epub_fetch_prev_page_chunk() == 0)
    {
        memset(current_book->pretty_page, 0, sizeof(current_book->pretty_page));
        epub_prettify_page();
        LOG_DBG("Pretty page: %s", current_book->pretty_page);
        return current_book->pretty_page;
    }
    LOG_DBG("Can't get page");
    return "";
}

int epub_open_book(book_entry_t *book)
{
    free(current_book);
    current_book = (current_book_t *)malloc(sizeof(current_book_t));

    current_book->state.title = strdup(book->title);
    current_book->state.chapter = -1;
    current_book->state.file_offset = 0;

    current_book->num_chapters = 0;
    current_book->chapter_list = NULL;
    current_book->current_chapter = NULL;
    current_book->chapter_filename = NULL;
    current_book->root_dir = book->root_dir;

    epub_parse_chapter_files(book->entry_point);
    return epub_get_next_chapter();
}

int epub_restore_book()
{
    book_entry_t *book = NULL;

    epub_get_current_book_state();
    LOG_DBG("Restore title %s", current_book->state.title);
    book = epub_get_book_entry_for_title(current_book->state.title);

    current_book->num_chapters = 0;
    current_book->chapter_list = NULL;
    current_book->current_chapter = NULL;
    current_book->chapter_filename = NULL;
    current_book->root_dir = book->root_dir;

    epub_parse_chapter_files(book->entry_point);
    epub_get_chapter_entry(current_book->state.chapter);

    return epub_get_chapter(current_book->state.chapter);
}

int epub_initialize()
{
    int ret = 0;

    LOG_DBG("Init SD card");
    ret = sd_initialize();
    if (ret)
    {
        LOG_ERR("Initializing the SD card failed");
        return ret;
    }

    ret = epub_get_epub_rootfiles();
        if (ret)
    {
        LOG_ERR("Fetching the EPUB rootfiles failed.");
        return ret;
    }

    epub_get_authors_and_titles();

    return ret;
}
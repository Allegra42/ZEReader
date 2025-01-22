#include <string.h>
#include <stdio.h>

#include "sd_card.h"
#include "epub.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(epub, CONFIG_ZEREADER_LOG_LEVEL);

static char epub_root_dir[EPUB_FILE_NUM_MAX][EPUB_FILE_LEN_MAX / 2];
static char epub_entry_points[EPUB_FILE_NUM_MAX][EPUB_FILE_LEN_MAX];
static char epub_titles[EPUB_FILE_NUM_MAX][EPUB_TITLE_AUTHOR_LEN_MAX];
static char epub_authors[EPUB_FILE_NUM_MAX][EPUB_TITLE_AUTHOR_LEN_MAX];
static char epub_current_book_chapter_list[EPUB_CHAPTERS_MAX][EPUB_FILE_LEN_MAX];
static char epub_current_page[EPUB_PAGE_SIZE];

char current_filename[EPUB_FILE_LEN_MAX];
static uint16_t found_books = 0;
size_t current_book = -1;
size_t current_file_offset = 0;
size_t current_chapter = 0;
uint16_t current_book_num_chapters = 0;

// This implementation is a first compromise in terms of time, effort and memory/search time optimization.
// Usually, the starting point (rootfile) of an ePub is called content.opf and located in the OEBPS
// directory. Both, the file name and the location are not required, but commonly used.
// In this case, the file can not be found with the current approach.
// The most correct approach would be searching for META-INF/container.xml which must be present to
// fulfill the ePub standard.
// It contains the relative path in the directory to the rootfile in such a format:
// <rootfile full-path="OEBPS/e9783641066314_content.opf" media-type="application/oebps-package+xml"/>
// As this would result in opening and parsing two file to get the relevant information,
// this naive approach is choosen to start with.
// https://www.w3.org/TR/epub-33/#sec-container-metainf
// https://cdn.standards.iteh.ai/samples/53255/6fd3b6154cf04b2da069a604a4bb76d8/ISO-IEC-TS-30135-1-2014.pdf
int epub_get_entry_points()
{

    uint16_t found = 0;
    int ret = 0;

    // Recursively searching content.opf files from SD card root is a memory efficiency nightmare
    // Instead, get top level folders and start searching the content.opf from there.
    char assumed_location[] = "/OEBPS/content.opf\0";
    uint16_t top_level_path_size = EPUB_FILE_LEN_MAX - strlen(assumed_location);

    size_t max_ls_content_size = EPUB_LSDIR_CHARS_MAX;
    char top_level_content[max_ls_content_size];
    ret = sd_card_list_files(NULL, top_level_content, &max_ls_content_size, true);
    if (ret != 0)
    {
        LOG_ERR("Listing top level files failed");
        return ret;
    }

    char *token = strtok(top_level_content, "\n");
    while (token)
    {
        // // Recursive search approach - kept here for reference
        // // Works but still inefficient and issues with large folders
        // ret = sd_card_list_files_match(1, EPUB_FILE_LEN_MAX, content_opf, token+7, "content.opf");
        // if (ret > 0) {
        //     LOG_DBG("Found: %s", content_opf[0]);
        //     // Found file, copy it to the permanent buffer
        //     strcpy(epub_entry_points[found], content_opf[0]);
        //     found++;
        // }

        // Remove [DIR ] and search for content.opf
        // Each top level folder should only contain one content.opf
        if (strstr(token, "[DIR ]") != 0)
        {
            // The prepending string [DIR ] <foldername> is always 7 chars long.
            // As strings in C are just pointers, we can just skip it by adding +7.
            // LOG_DBG("Found path: %s", token + 7);

            // Remove \r from top level path
            char top_level_path[top_level_path_size];
            strncpy(top_level_path, token + 7, strlen(token + 7));
            top_level_path[strlen(token + 7) - 1] = 0;

            if (strlen(top_level_path) + strlen(assumed_location) > EPUB_FILE_LEN_MAX)
            {
                LOG_DBG("Resulting path is too long!");
                return -1;
            }

            char assumed_path[EPUB_FILE_LEN_MAX] = {0};
            strncat(assumed_path, top_level_path, strlen(top_level_path));
            strncat(assumed_path, assumed_location, strlen(assumed_location));

            struct fs_file_t f_entry;
            fs_file_t_init(&f_entry);

            ret = sd_card_open(assumed_path, &f_entry);
            if (ret == 0)
            {
                // File found, add it to the list
                strncpy(epub_entry_points[found], assumed_path, strlen(assumed_path));
                strncpy(epub_root_dir[found], top_level_path, strlen(top_level_path));
                sd_card_close(&f_entry);
                found++;
            }
        }
        token = strtok(NULL, "\n");
    }

    found_books = found;
    return ret;
}

void epub_content_opf_metadata_get_element(const char *search_tag, const char *filename,
                                           size_t file_read_size, char *element, size_t max_element_size)
{
    char *delim = ">";
    char read_buffer[file_read_size];
    uint16_t len_search_tag = strlen(search_tag) + 2;

    sd_card_open_read_close(filename, read_buffer, &file_read_size);

    char *token = strtok(read_buffer, delim);
    while (token)
    {
        if (strstr(token, search_tag) != 0)
        {
            token = strtok(NULL, delim);

            int token_len = strlen(token);
            int copy_len = (token_len < max_element_size) ? token_len : max_element_size;

            token[token_len - len_search_tag] = 0;

            strncpy(element, token, copy_len);
            element[copy_len] = 0;
        }

        token = strtok(NULL, delim);
    }
}

void epub_parse_book_titles()
{
    const char *search_tag = "dc:title";
    size_t file_read_size = 800;

    for (size_t i = 0; i < found_books; i++)
    {
        epub_content_opf_metadata_get_element(search_tag, epub_entry_points[i], file_read_size,
                                              epub_titles[i], EPUB_TITLE_AUTHOR_LEN_MAX);
        LOG_DBG("%s", epub_titles[i]);
    }
}

char (*epub_get_title_list(void))[EPUB_TITLE_AUTHOR_LEN_MAX]
{
    return epub_titles;
}

void epub_parse_book_authors()
{
    const char *search_tag = "dc:creator";
    size_t file_read_size = 800;

    for (size_t i = 0; i < found_books; i++)
    {
        epub_content_opf_metadata_get_element(search_tag, epub_entry_points[i], file_read_size,
                                              epub_authors[i], EPUB_TITLE_AUTHOR_LEN_MAX);
        LOG_DBG("%s", epub_authors[i]);
    }
}

char (*epub_get_author_list(void))[EPUB_TITLE_AUTHOR_LEN_MAX]
{
    return epub_authors;
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

    // Reset chapter counter when parsing a new book
    current_book_num_chapters = 0;

    while (read_size == 800)
    {
        ret = sd_card_open_read_at_offset_close(content_opf, &offset, buf, &read_size);
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
                    strncpy(epub_current_book_chapter_list[current_book_num_chapters], path, path_len);
                    epub_current_book_chapter_list[current_book_num_chapters][path_len] = 0;

                    LOG_DBG("chapter path: %s", epub_current_book_chapter_list[current_book_num_chapters]);
                    current_book_num_chapters++;
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

void epub_build_chapter_path(char *path)
{
    memset(path, 0, EPUB_FILE_LEN_MAX);
    strncpy(path, epub_root_dir[current_book], strlen(epub_root_dir[current_book]));
    strcat(path, "/OEBPS/");
    strcat(path, epub_current_book_chapter_list[current_chapter]);

    LOG_DBG("Full path %s", path);
}

bool is_html(char *path)
{
    return strstr(path, "html");
}

int epub_get_next_chapter()
{
    current_file_offset = 0;
    LOG_DBG("Get next chapter");
    if (current_chapter == current_book_num_chapters - 1)
    {
        LOG_DBG("Book finished!");
        return 0;
    }
    current_chapter++;

    LOG_DBG("before get path");
    epub_build_chapter_path(current_filename);

    if (!is_html(current_filename))
    {
        epub_get_next_chapter();
    }
}

int epub_get_prev_chapter()
{
    if (current_chapter == 0)
    {
        LOG_DBG("Already at the beginning");
        return 0;
    }
    current_chapter--;

    epub_build_chapter_path(current_filename);

    if (!is_html(current_filename))
    {
        epub_get_prev_chapter();
    }
    else
    {
        // TODO does not work as expected right now
        LOG_DBG("Opening previous file %s", current_filename);
        sd_card_tell_end_offset(current_filename, &current_file_offset);
        LOG_DBG("End offset of the prev chapter: %d", current_file_offset);
    }
}

int epub_parse_next_page()
{
    size_t read_size = EPUB_PAGE_SIZE - 1;

    int ret = sd_card_open_read_at_offset_close(current_filename, &current_file_offset,
                                                epub_current_page, &read_size);
    epub_current_page[read_size] = 0;

    LOG_DBG("%s", epub_current_page);

    if (read_size < (EPUB_PAGE_SIZE - 1))
    {
        LOG_DBG("Open next chapter");
        epub_get_next_chapter();
    }

    return ret;
}

char *epub_get_next_page()
{

    if (epub_parse_next_page() == 0)
    {
        return epub_current_page;
    }
    LOG_DBG("Can't get page");
    return "";
}

int epub_parse_prev_page()
{
    size_t read_size = (EPUB_PAGE_SIZE - 1);
    current_file_offset -= 2 * read_size;

    int ret = sd_card_open_read_at_offset_close(current_filename, &current_file_offset,
                                                epub_current_page, &read_size);
    epub_current_page[read_size] = 0;
    LOG_DBG("%s", epub_current_page);

    if (ret < 0)
    {
        LOG_DBG("Open previous chapter");
        epub_get_prev_chapter();
    }

    return ret;
}

char *epub_get_prev_page()
{

    if (epub_parse_prev_page() == 0)
    {
        return epub_current_page;
    }
    LOG_DBG("Can't get page");
    return "";
}

int epub_open_book(uint16_t num)
{

    current_book = num;
    current_chapter = -1;
    current_file_offset = 0;
    current_book_num_chapters = 0;
    memset(current_filename, 0, EPUB_FILE_LEN_MAX);

    epub_parse_chapter_files(epub_entry_points[current_book]);
    epub_get_next_chapter();
}

int epub_initialize()
{
    int ret = 0;

    ret = sd_card_init();
    if (ret)
    {
        LOG_ERR("Initializing the SD card failed");
        return ret;
    }

    // ret =
    epub_get_entry_points();
    // if (ret) {
    //     LOG_ERR("Parse EBOOK entry points failed");
    //     return ret;
    // }

    // ret =
    epub_parse_book_titles();
    // if (ret) {
    //     LOG_ERR("Parse book titles failed");
    //     return ret;
    // }

    epub_parse_book_authors();
}
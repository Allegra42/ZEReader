/*
 * SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <epub/epub.h>
#include <sd/sd.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(epub, CONFIG_ZEREADER_LOG_LEVEL);

K_MUTEX_DEFINE(epub_mutex);

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

void epub_update_page(int32_t update)
{
  if (current_book)
  {
    current_book->state.page = current_book->state.page + update;
  }
}

int32_t epub_get_page()
{
  if (current_book)
  {
    return current_book->state.page;
  }
  return 0;
}

int epub_write_current_book_state()
{
  k_mutex_lock(&epub_mutex, K_FOREVER);

  if (!current_book)
  {
    LOG_DBG("No current book");
    /* There is nothing to do, which is always successful */
    k_mutex_unlock(&epub_mutex);
    return 0;
  }
  LOG_DBG("Writing the book's state");

  size_t to_write;
  char state_string[EPUB_STATE_STRING_SIZE];

  sprintf(state_string, "%s\n%zu\n%zu\n", current_book->state.title, current_book->state.chapter, current_book->state.page);
  to_write = strlen(state_string);

  int ret = sd_write_chunk(STATE_FILE, state_string, &to_write);
  if (ret)
  {
    LOG_ERR("Writing the book's state file failed!");
  }
  k_mutex_unlock(&epub_mutex);
  return ret;
}

current_book_t *epub_get_current_book_state()
{
  k_mutex_lock(&epub_mutex, K_FOREVER);

  size_t file_size;
  char *buf = sd_read_whole_file(STATE_FILE, &file_size);
  if (buf == NULL)
  {
    LOG_INF("Could not read current book state file: %s", STATE_FILE);
    k_mutex_unlock(&epub_mutex);
    return NULL;
  }

  LOG_DBG("Current state:\n %s", buf);

  char *title_read = 0; // Renamed to avoid confusion with allocated title
  int32_t chapter = 0;
  size_t page = 0;

  char *saveptr;
  char *token = strtok_r(buf, "\n", &saveptr);
  if (token != NULL)
  {
    title_read = strdup(token); // Store duplicated title in a temporary var
    LOG_DBG("Title: %s", title_read);
  }

  token = strtok_r(NULL, "\n", &saveptr);
  if (token != NULL)
  {
    chapter = strtol(token, NULL, 10);
    LOG_DBG("Chapter: %d", chapter);
  }

  token = strtok_r(NULL, "\n", &saveptr);
  if (token != NULL)
  {
    page = strtoul(token, NULL, 10);
    LOG_DBG("Page: %d", page);
  }

  current_book_t *current_book_local = malloc(sizeof(current_book_t));
  if (current_book_local)
  {
    current_book_local->state.title = strdup(title_read ? title_read : ""); // Duplicate title_read or empty string
    current_book_local->state.chapter = chapter;
    current_book_local->state.page = page;
  }

  free(buf); // Free the dynamically allocated buffer
  if (title_read)
    free(title_read); // Free the temporary duplicated title

  k_mutex_unlock(&epub_mutex);
  return current_book_local;
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

chapter_list_t *epub_get_chapter_list()
{
  return current_book->chapter_list;
}

char *epub_content_opf_metadata_get_element(const char *search_tag, const char *filename,
                                            size_t file_read_size)
{
  char *delim = ">";
  size_t file_size;
  char *read_buffer = sd_read_whole_file(filename, &file_size);
  if (read_buffer == NULL)
  {
    LOG_ERR("Failed to read whole file %s", filename);
    return NULL;
  }

  uint16_t len_search_tag = strlen(search_tag);
  char *saveptr;
  char *token = strtok_r(read_buffer, delim, &saveptr);
  char *heap_str = NULL; // To store the result before freeing read_buffer

  while (token)
  {
    if (strstr(token, search_tag) != 0)
    {
      token[strlen(token) - len_search_tag] = 0;
      heap_str = strdup(token);
      break; // Found and duplicated, exit loop
    }
    token = strtok_r(NULL, delim, &saveptr);
  }

  free(read_buffer); // Free the dynamically allocated buffer
  return heap_str;
}

char *epub_container_xml_get_rootpath(const char *folder, const char *filepath)
{
  size_t file_size;
  char *buffer = sd_read_whole_file(filepath, &file_size);
  if (buffer == NULL)
  {
    LOG_ERR("Failed to read whole file %s", filepath);
    return NULL;
  }

  char *full_path_attr = strstr(buffer, "full-path=");
  if (full_path_attr != NULL)
  {
    char *path_start = strchr(full_path_attr, '"');
    if (path_start != NULL)
    {
      path_start++; // Move past the opening quote
      char *path_end = strchr(path_start, '"');
      if (path_end != NULL)
      {
        int path_len = path_end - path_start;
        if (path_len <= 0)
        {
          free(buffer); // Free buffer before returning
          return NULL;  // Empty path
        }

        LOG_DBG("Found 'full-path' value: %.*s", path_len, path_start);

        // Calculate required size for rootpath: "/SD:/" + folder + "/" + path_value + null_terminator
        size_t rootpath_len = strlen("/SD:/") + strlen(folder) + strlen("/") + path_len + 1;
        char *rootpath = (char *)malloc(rootpath_len);
        if (rootpath == NULL)
        {
          LOG_ERR("Failed to allocate memory for rootpath");
          free(buffer); // Free buffer before returning
          return NULL;
        }

        int written = snprintf(rootpath, rootpath_len, "/SD:/%s/%.*s", folder, path_len, path_start);
        if (written < 0 || written >= rootpath_len)
        {
          LOG_ERR("snprintf failed or truncated rootpath");
          free(rootpath);
          free(buffer); // Free buffer before returning
          return NULL;
        }

        LOG_DBG("Constructed rootpath: %s", rootpath);
        free(buffer); // Free buffer before returning
        return rootpath;
      }
    }
  }
  free(buffer); // Free buffer before returning
  return NULL;
}

int epub_get_epub_rootfiles()
{
  k_mutex_lock(&epub_mutex, K_FOREVER);

  int ret;
  int found = 0;

  char *container_xml = "/META-INF/container.xml";
  char *book_path;
  char *rootpath;

  struct fs_dir_t dir_obj;
  // Removed static as this caused thread safety issues
  struct fs_dirent entry;

  fs_dir_t_init(&dir_obj);

  ret = fs_opendir(&dir_obj, "/SD:/");
  if (ret)
  {
    LOG_ERR("Open root directory failed!");
    k_mutex_unlock(&epub_mutex);
    return ret;
  }

  while (true)
  {
    ret = fs_readdir(&dir_obj, &entry);
    if (ret)
    {
      LOG_DBG("Could not read directory");
      k_mutex_unlock(&epub_mutex);
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
        LOG_DBG("Found valid book %s, %s", book_path, entry.name);

        rootpath = epub_container_xml_get_rootpath(entry.name, book_path);
        if (rootpath != NULL)
        {
          LOG_DBG("Found rootfile path: %s", rootpath);

          book_entry_t *book = epub_add_book_entry();
          book->number = found;
          book->entry_point = strdup(rootpath);
          LOG_DBG("Entry point: %s", book->entry_point);
          LOG_DBG("Root dir: %s", entry.name);

          // Usually the rootpath should look like
          // /SD:/<bookname>/OEBPS/content.opf
          // and as the manifest structure in the content.opf uses a
          // file-relative addressing, contents are usually stored in
          // /SD:/<bookname>/OEBPS/
          // But some epubs, especially older free resources do not follow
          // this convention, thus hardcoding this assumption does not work.
          // A better approach is using the directory containing the content.opf
          // as a "content root path".
          // 11 is the length of "/content.opf"
          book->content_dir = strndup(book->entry_point, strlen(book->entry_point) - 12);
          LOG_DBG("Content dir: %s", book->content_dir);

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
    k_mutex_unlock(&epub_mutex);
    return ret;
  }

  k_mutex_unlock(&epub_mutex);
  return 0;
}

void epub_get_authors_and_titles()
{
  k_mutex_lock(&epub_mutex, K_FOREVER);
  size_t file_read_size = EPUB_OPF_READ_SIZE;
  const char *search_creator = "</dc:creator";
  const char *search_title = "</dc:title";
  char *element;

  book_list_t *current_elem = book_list;
  while (current_elem != NULL)
  {
    book_entry_t *book = current_elem->book;
    element = epub_content_opf_metadata_get_element(search_creator, book->entry_point, file_read_size);
    book->author = element;

    element = epub_content_opf_metadata_get_element(search_title, book->entry_point, file_read_size);
    book->title = element;
    current_elem = current_elem->next;
  }
  k_mutex_unlock(&epub_mutex);
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
  size_t file_size;
  char *file_content = sd_read_whole_file(content_opf, &file_size);
  if (file_content == NULL)
  {
    LOG_ERR("Failed to read whole file %s", content_opf);
    return -EIO;
  }

  const char *search_tag = "href=\"";
  uint16_t len_search_tag = strlen(search_tag);

  char *buffer_ptr = file_content; // Start processing from the beginning of the content

  while (true)
  {
    char *found_href = strstr(buffer_ptr, search_tag);
    if (found_href != NULL)
    {
      char *path_start = found_href + len_search_tag;
      char *path_end = strchr(path_start, '"');

      if (path_end != NULL)
      {
        int path_len = path_end - path_start;

        if (path_len >= EPUB_FILE_LEN_MAX)
        {
          LOG_ERR("Found path is too long: %.*s", path_len, path_start);
          buffer_ptr = path_end + 1; // Skip this long path and continue
          continue;
        }
        else if (path_len > 0)
        {
          chapter_entry_t *chapter = epub_add_chapter_entry();
          if (chapter == NULL)
          {
            LOG_ERR("Failed to create new chapter entry");
            ret = -ENOMEM;
            break; // Exit loop on error
          }
          size_t chapter_path_len = strlen(current_book->content_dir) + path_len + 2;
          chapter->path = (char *)malloc(chapter_path_len);
          if (chapter->path == NULL)
          {
            LOG_ERR("Failed to allocate memory for chapter path");
            ret = -ENOMEM;
            break; // Exit loop on error
          }
          snprintf(chapter->path, chapter_path_len, "%s/%.*s",
                   current_book->content_dir, path_len, path_start);
          chapter->number = current_book->num_chapters;

          LOG_DBG("chapter path: %s", chapter->path);
          current_book->num_chapters++;
        }
        buffer_ptr = path_end + 1;
      }
      else
      {
        // This case should ideally not happen if the file was read whole and is well-formed HTML/XML
        // but if it does, we break to avoid infinite loop.
        LOG_ERR("Malformed HTML/XML: href found without closing quote.");
        break;
      }
    }
    else
    {
      break; // No more hrefs found
    }
  }

  free(file_content); // Free the dynamically allocated buffer
  return ret;
}

bool is_html(char *path)
{
  // Accepts .htm, .html, and .htmlx files as valid HTML
  return strstr(path, "htm");
}

static size_t _epub_parse_html(const char *input, size_t input_len, char *output, size_t output_max_len)
{
  size_t input_idx = 0;
  size_t output_idx = 0;
  bool in_tag = false;
  const char *ignored_tags[] = {"script", "style", "head", "header"};

  while (input_idx < input_len && output_idx < (output_max_len - 1))
  {
    if (in_tag)
    {
      if (input[input_idx] == '>')
      {
        in_tag = false;
      }
    }
    else if (input[input_idx] == '<')
    {
      in_tag = true;

      // Handle tags that force a newline
      if (strncmp(&input[input_idx], "</p>", 4) == 0 ||
          strncmp(&input[input_idx], "</div>", 6) == 0 ||
          strncmp(&input[input_idx], "</li>", 5) == 0 ||
          strncmp(&input[input_idx], "</h1>", 5) == 0 ||
          strncmp(&input[input_idx], "</h2>", 5) == 0 ||
          strncmp(&input[input_idx], "</h3>", 5) == 0 ||
          strncmp(&input[input_idx], "<br", 3) == 0)
      {
        // Trim trailing space before adding newline
        if (output_idx > 0 && output[output_idx - 1] == ' ')
        {
          output_idx--;
        }
        // Add newline if not already on one
        if (output_idx > 0 && output[output_idx - 1] != '\n')
        {
          output[output_idx++] = '\n';
        }
      }
      else
      {
        // Check for ignored tags and skip their content
        for (size_t i = 0; i < sizeof(ignored_tags) / sizeof(ignored_tags[0]); ++i)
        {
          char open_tag[16];
          snprintf(open_tag, sizeof(open_tag), "<%s", ignored_tags[i]);
          if (strncmp(&input[input_idx], open_tag, strlen(open_tag)) == 0)
          {
            char close_tag[16];
            snprintf(close_tag, sizeof(close_tag), "</%s>", ignored_tags[i]);
            const char *end = strstr(&input[input_idx], close_tag);
            if (end)
            {
              input_idx = (end - input) + strlen(close_tag) - 1; // -1 for loop increment
              in_tag = false;                                    // We are now outside of any tag
              goto loop_end;
            }
          }
        }
      }
    }
    else
    { // Not in a tag, this is content
      if (isspace((unsigned char)input[input_idx]))
      {
        // Collapse whitespace: only add a space if the last char wasn't whitespace and not at the beginning.
        if (output_idx > 0 && output[output_idx - 1] != ' ' && output[output_idx - 1] != '\n')
        {
          output[output_idx++] = ' ';
        }
      }
      else if (input[input_idx] == '&')
      {
        // Handle common HTML entities
        if (strncmp(&input[input_idx], "&nbsp;", 6) == 0)
        {
          if (output_idx > 0 && output[output_idx - 1] != ' ' && output[output_idx - 1] != '\n')
          {
            output[output_idx++] = ' ';
          }
          input_idx += 5;
        }
        else if (strncmp(&input[input_idx], "&amp;", 5) == 0)
        {
          output[output_idx++] = '&';
          input_idx += 4;
        }
        else if (strncmp(&input[input_idx], "&lt;", 4) == 0)
        {
          output[output_idx++] = '<';
          input_idx += 3;
        }
        else if (strncmp(&input[input_idx], "&gt;", 4) == 0)
        {
          output[output_idx++] = '>';
          input_idx += 3;
        }
        else if (strncmp(&input[input_idx], "&quot;", 6) == 0)
        {
          output[output_idx++] = '"';
          input_idx += 5;
        }
        else if (strncmp(&input[input_idx], "&apos;", 6) == 0)
        {
          output[output_idx++] = '\'';
          input_idx += 5;
        }
        else
        {
          output[output_idx++] = input[input_idx];
        }
      }
      else
      {
        output[output_idx++] = input[input_idx];
      }
    }
  loop_end:
    input_idx++;
  }

  // Trim trailing whitespace from the final output
  while (output_idx > 0 && isspace((unsigned char)output[output_idx - 1]))
  {
    output_idx--;
  }

  output[output_idx] = '\0';
  return output_idx;
}

static int epub_prettify_full_chapter(const char *raw_content, size_t raw_content_size, char **prettified_content_out, size_t *prettified_content_size_out)
{
  size_t estimated_output_size = raw_content_size + 1; // +1 for null terminator

  char *prettified_buffer = (char *)malloc(estimated_output_size);
  if (prettified_buffer == NULL)
  {
    LOG_ERR("Failed to allocate memory for prettified chapter content.");
    return -ENOMEM;
  }

  size_t actual_prettified_len = _epub_parse_html(raw_content, raw_content_size, prettified_buffer, estimated_output_size);

  *prettified_content_out = prettified_buffer;
  *prettified_content_size_out = actual_prettified_len;

  return 0;
}

int epub_get_next_chapter()
{
  LOG_DBG("Get next chapter");

  // Free previous chapter's raw and prettified content if it exists
  if (current_book->chapter_raw_content)
  {
    free(current_book->chapter_raw_content);
    current_book->chapter_raw_content = NULL;
    current_book->chapter_raw_content_size = 0;
  }
  if (current_book->chapter_prettified_content)
  {
    free(current_book->chapter_prettified_content);
    current_book->chapter_prettified_content = NULL;
    current_book->chapter_prettified_content_size = 0;
  }

  while (true)
  {
    if (current_book->state.chapter >= current_book->num_chapters - 1)
    {
      LOG_DBG("Book finished!");
      return -ENODATA;
    }

    if (current_book->current_chapter == NULL)
    {
      // Set linked list pointer to the first element
      current_book->current_chapter = current_book->chapter_list;
    }
    else
    {
      current_book->current_chapter = current_book->current_chapter->next;
    }

    if (current_book->current_chapter == NULL)
    {
      LOG_DBG("Book finished!");
      return -ENODATA;
    }

    current_book->state.chapter++;
    current_book->state.page = 0;

    if (!is_html(current_book->current_chapter->chapter->path))
    {
      continue;
    }
    else
    {
      // Load the entire chapter raw content
      current_book->chapter_raw_content = sd_read_whole_file(
          current_book->current_chapter->chapter->path,
          &current_book->chapter_raw_content_size);
      if (!current_book->chapter_raw_content)
      {
        LOG_ERR("Failed to read whole chapter file: %s", current_book->current_chapter->chapter->path);
        return -EIO;
      }
      LOG_DBG("Loaded raw chapter: %s, size: %zu", current_book->current_chapter->chapter->path, current_book->chapter_raw_content_size);

      // Prettify the entire chapter content
      int ret = epub_prettify_full_chapter(
          current_book->chapter_raw_content,
          current_book->chapter_raw_content_size,
          &current_book->chapter_prettified_content,
          &current_book->chapter_prettified_content_size);

      // Free raw content after prettification as it's no longer needed
      free(current_book->chapter_raw_content);
      current_book->chapter_raw_content = NULL;

      if (ret != 0)
      {
        LOG_ERR("Failed to prettify chapter: %s", current_book->current_chapter->chapter->path);
        return ret;
      }

      LOG_DBG("Prettified chapter size: %zu", current_book->chapter_prettified_content_size);
      if (current_book->chapter_prettified_content_size > 0)
      {
        break; // Found a chapter with content
      }
    }
  }
  return 0;
}

int epub_get_chapter(size_t index)
{
  LOG_DBG("Get chapter %d", index);
  if (current_book->state.chapter >= current_book->num_chapters - 1)
  {
    LOG_DBG("Book finished!");
    return 0;
  }
  // Set linked list pointer to the first element
  current_book->current_chapter = current_book->chapter_list;

  for (size_t i = 0; i < index; i++)
  {
    if (current_book->current_chapter->next != NULL)
    {
      current_book->current_chapter = current_book->current_chapter->next;
    }
    else
    {
      // Index is out of bounds
      return -1;
    }
  }

  return 0;
}

int epub_get_prev_chapter()
{
  LOG_DBG("Get prev chapter");

  // Free previous chapter's raw and prettified content if it exists
  if (current_book->chapter_raw_content)
  {
    free(current_book->chapter_raw_content);
    current_book->chapter_raw_content = NULL;
    current_book->chapter_raw_content_size = 0;
  }
  if (current_book->chapter_prettified_content)
  {
    free(current_book->chapter_prettified_content);
    current_book->chapter_prettified_content = NULL;
    current_book->chapter_prettified_content_size = 0;
  }

  while (true)
  {
    if (current_book->state.chapter == 0)
    {
      LOG_DBG("Already at the beginning");
      return -ENODATA; // Indicate no previous chapter
    }

    if (current_book->current_chapter->prev != NULL)
    {
      current_book->current_chapter = current_book->current_chapter->prev;
      current_book->state.chapter--;
      current_book->state.page = 0;

      if (!is_html(current_book->current_chapter->chapter->path))
      {
        continue;
      }
      else
      {
        // Load the entire chapter raw content
        current_book->chapter_raw_content = sd_read_whole_file(
            current_book->current_chapter->chapter->path,
            &current_book->chapter_raw_content_size);
        if (!current_book->chapter_raw_content)
        {
          LOG_ERR("Failed to read whole chapter file: %s", current_book->current_chapter->chapter->path);
          return -EIO;
        }
        LOG_DBG("Loaded raw chapter: %s, size: %zu", current_book->current_chapter->chapter->path, current_book->chapter_raw_content_size);

        // Prettify the entire chapter content
        int ret = epub_prettify_full_chapter(
            current_book->chapter_raw_content,
            current_book->chapter_raw_content_size,
            &current_book->chapter_prettified_content,
            &current_book->chapter_prettified_content_size);

        // Free raw content after prettification as it's no longer needed
        free(current_book->chapter_raw_content);
        current_book->chapter_raw_content = NULL;

        if (ret != 0)
        {
          LOG_ERR("Failed to prettify chapter: %s", current_book->current_chapter->chapter->path);
          return ret;
        }

        LOG_DBG("Prettified chapter size: %zu", current_book->chapter_prettified_content_size);
        LOG_DBG("Opening previous file %s", current_book->current_chapter->chapter->path);

        if (current_book->chapter_prettified_content_size > 0)
        {
          break; // Found a chapter with content
        }
      }
    }
  }
  return 0;
}

const char *epub_get_current_chapter_content(void)
{
  if (current_book)
  {
    return current_book->chapter_prettified_content;
  }
  return NULL;
}

int epub_free_current_book_resources()
{
  k_mutex_lock(&epub_mutex, K_FOREVER);

  if (current_book == NULL)
  {
    k_mutex_unlock(&epub_mutex);
    return 0;
  }

  if (current_book->chapter_filename)
  {
    free(current_book->chapter_filename);
    current_book->chapter_filename = NULL;
  }

  if (current_book->state.title)
  {
    free(current_book->state.title);
    current_book->state.title = NULL;
  }

  if (current_book->content_dir)
  {
    free(current_book->content_dir);
    current_book->content_dir = NULL;
  }

  if (current_book->chapter_raw_content)
  {
    free(current_book->chapter_raw_content);
    current_book->chapter_raw_content = NULL;
  }
  if (current_book->chapter_prettified_content)
  {
    free(current_book->chapter_prettified_content);
    current_book->chapter_prettified_content = NULL;
  }

  chapter_list_t *current = current_book->chapter_list;
  while (current != NULL)
  {
    chapter_list_t *next = current->next;
    if (current->chapter)
    {
      if (current->chapter->path)
      {
        free(current->chapter->path);
        current->chapter->path = NULL;
      }
      free(current->chapter);
      current->chapter = NULL;
    }
    free(current);
    current = next;
  }

  current_book->chapter_list = NULL;
  current_book->current_chapter = NULL;

  free(current_book);
  current_book = NULL;

  k_mutex_unlock(&epub_mutex);
  return 0;
}

int epub_open_book(book_entry_t *book)
{
  LOG_DBG("epub_open_book: Start");
  k_mutex_lock(&epub_mutex, K_FOREVER);
  LOG_DBG("epub_open_book: Mutex locked");

  epub_free_current_book_resources();
  LOG_DBG("epub_open_book: Resources freed");

  current_book = (current_book_t *)malloc(sizeof(current_book_t));
  if (current_book == NULL)
  {
    LOG_ERR("epub_open_book: Failed to allocate current_book");
    k_mutex_unlock(&epub_mutex);
    return -ENOMEM;
  }
  LOG_DBG("epub_open_book: current_book allocated");

  current_book->state.title = strdup(book->title);
  if (current_book->state.title == NULL)
  {
    LOG_ERR("epub_open_book: Failed to strdup title");
    free(current_book);
    current_book = NULL;
    k_mutex_unlock(&epub_mutex);
    return -ENOMEM;
  }
  LOG_DBG("epub_open_book: title strdup'd");

  current_book->state.chapter = -1;
  current_book->state.page = 0;
  current_book->num_chapters = 0;
  current_book->chapter_list = NULL;
  current_book->current_chapter = NULL;
  current_book->chapter_filename = NULL;
  current_book->chapter_raw_content = NULL;
  current_book->chapter_raw_content_size = 0;
  current_book->chapter_prettified_content = NULL;
  current_book->chapter_prettified_content_size = 0;

  current_book->content_dir = strdup(book->content_dir);
  if (current_book->content_dir == NULL)
  {
    LOG_ERR("epub_open_book: Failed to strdup content_dir");
    free(current_book->state.title);
    free(current_book);
    current_book = NULL;
    k_mutex_unlock(&epub_mutex);
    return -ENOMEM;
  }
  LOG_DBG("epub_open_book: content_dir strdup'd");

  epub_parse_chapter_files(book->entry_point);
  LOG_DBG("epub_open_book: Chapter files parsed");
  int ret = epub_get_next_chapter();
  LOG_DBG("epub_open_book: Next chapter retrieved, ret=%d", ret);

  k_mutex_unlock(&epub_mutex);
  LOG_DBG("epub_open_book: Mutex unlocked");
  return ret;
}

int epub_restore_book()
{
  k_mutex_lock(&epub_mutex, K_FOREVER);

  book_entry_t *book = NULL;

  if (current_book)
  {
    epub_free_current_book_resources();
  }
  current_book = epub_get_current_book_state();
  if (!current_book)
  {
    LOG_INF("No current book state found.");
    k_mutex_unlock(&epub_mutex);
    return -ENOENT;
  }

  LOG_DBG("Restore title %s", current_book->state.title);
  book = epub_get_book_entry_for_title(current_book->state.title);

  current_book->num_chapters = 0;
  current_book->chapter_list = NULL;
  current_book->current_chapter = NULL;
  current_book->chapter_filename = NULL;
  current_book->chapter_raw_content = NULL;
  current_book->chapter_raw_content_size = 0;
  current_book->chapter_prettified_content = NULL;
  current_book->chapter_prettified_content_size = 0;
  current_book->content_dir = strdup(book->content_dir);

  epub_parse_chapter_files(book->entry_point);

  int ret = epub_get_chapter(current_book->state.chapter);

  // Load the entire chapter raw content for the restored chapter
  if (current_book->current_chapter && is_html(current_book->current_chapter->chapter->path))
  {
    current_book->chapter_raw_content = sd_read_whole_file(
        current_book->current_chapter->chapter->path,
        &current_book->chapter_raw_content_size);
    if (!current_book->chapter_raw_content)
    {
      LOG_ERR("Failed to read whole chapter file for restored book: %s", current_book->current_chapter->chapter->path);
      return -EIO;
    }
    LOG_DBG("Loaded raw chapter for restore: %s, size: %zu", current_book->current_chapter->chapter->path, current_book->chapter_raw_content_size);

    // Prettify the entire chapter content
    int pret_ret = epub_prettify_full_chapter(
        current_book->chapter_raw_content,
        current_book->chapter_raw_content_size,
        &current_book->chapter_prettified_content,
        &current_book->chapter_prettified_content_size);
    if (pret_ret != 0)
    {
      LOG_ERR("Failed to prettify chapter for restored book: %s", current_book->current_chapter->chapter->path);
      free(current_book->chapter_raw_content);
      current_book->chapter_raw_content = NULL;
      return pret_ret;
    }
    // Free raw content after prettification as it's no longer needed
    free(current_book->chapter_raw_content);
    current_book->chapter_raw_content = NULL;

    LOG_DBG("Restored chapter: %s, prettified size: %zu, page: %zu", current_book->current_chapter->chapter->path, current_book->chapter_prettified_content_size, current_book->state.page);
  }
  else if (current_book->current_chapter && !is_html(current_book->current_chapter->chapter->path))
  {
    // If the restored chapter is not HTML, advance to the next HTML chapter
    epub_get_next_chapter();
  }
  // else current_book->current_chapter is NULL, error already logged by epub_get_chapter_state()
  k_mutex_unlock(&epub_mutex);
  return ret;
}

int epub_destroy_book_list()
{
  k_mutex_lock(&epub_mutex, K_FOREVER);

  book_list_t *current = book_list;
  while (current != NULL)
  {
    book_list_t *next = current->next;
    if (current->book)
    {
      if (current->book->entry_point)
      {
        free(current->book->entry_point);
      }
      if (current->book->content_dir)
      {
        free(current->book->content_dir);
      }
      if (current->book->author)
      {
        free(current->book->author);
      }
      if (current->book->title)
      {
        free(current->book->title);
      }
      free(current->book);
    }
    free(current);
    current = next;
  }
  book_list = NULL;

  k_mutex_unlock(&epub_mutex);
  return 0;
}

int epub_initialize()
{
  k_mutex_lock(&epub_mutex, K_FOREVER);

  int ret = 0;

  LOG_DBG("Init SD card");
  ret = sd_initialize();
  if (ret)
  {
    LOG_ERR("Initializing the SD card failed");
    k_mutex_unlock(&epub_mutex);
    return ret;
  }

  ret = epub_get_epub_rootfiles();
  if (ret)
  {
    LOG_ERR("Fetching the EPUB rootfiles failed.");
    k_mutex_unlock(&epub_mutex);
    return ret;
  }

  epub_get_authors_and_titles();

  k_mutex_unlock(&epub_mutex);
  return ret;
}

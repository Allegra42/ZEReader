/*
 * SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#include <epub/epub.h>

#include <ui/ui.h>
#include <ui/fonts/notoserif_14.h>
#include <ui/logo/zereaderlogomarx.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ui, CONFIG_ZEREADER_LOG_LEVEL);

LV_FONT_DECLARE(notoserif_14);

const struct device *display_dev;

static lv_obj_t *button_1;
static lv_obj_t *button_1_label;

static lv_obj_t *button_2;
static lv_obj_t *button_2_label;

static lv_obj_t *button_3;
static lv_obj_t *button_3_label;

static lv_obj_t *button_4;
static lv_obj_t *button_4_label;

static lv_obj_t *text_area;

static lv_obj_t *logo;

static lv_obj_t *book_roller;

static lv_style_t font_style;

uint8_t page_ctr;

void zereader_print_prev_page();
void zereader_print_next_page();

static void book_roller_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *obj = lv_event_get_target_obj(e);
	context_t *context = lv_event_get_user_data(e);

	uint32_t book_nr = lv_roller_get_selected(obj);

	if (code == LV_EVENT_VALUE_CHANGED)
	{
		*context = READING;
		lv_label_set_text(button_1_label, BT_PREV);
		lv_label_set_text(button_2_label, BT_MENU);
		lv_label_set_text(button_3_label, BT_NONE);
		lv_label_set_text(button_4_label, BT_NEXT);
		lv_obj_del(book_roller);
		book_roller = NULL;
		epub_open_book(epub_get_book_entry_for_num(book_nr));
		zereader_print_next_page();
		epub_write_current_book_state();
		// epub_get_current_book_state();
	}
}

void zereader_show_bookmenu(context_t *context)
{
	char book_entry[50];
	char book_list[1000];

	memset(book_entry, 0, 50);
	memset(book_list, 0, 1000);

	book_roller = lv_roller_create(lv_screen_active());
	book_list_t *books = epub_get_book_list();

	while (books != NULL)
	{
		LOG_DBG("NR: %d - %s - %s - %s - %s", books->book->number, books->book->title, books->book->author, books->book->root_dir, books->book->entry_point);
		snprintf(book_entry, 49, "%d - %s - %s", books->book->number, books->book->author, books->book->title);
		LOG_DBG("book_entry: %s", book_entry);
		strcat(book_entry, "\n");
		strcat(book_list, book_entry);
		books = books->next;
	}

	lv_roller_set_options(book_roller, book_list, LV_ROLLER_MODE_INFINITE);

	lv_roller_set_visible_row_count(book_roller, 8);
	lv_obj_center(book_roller);

	lv_style_selector_t selector = LV_PART_MAIN;
	lv_obj_set_style_anim_time(book_roller, 0, selector);

	lv_obj_add_event_cb(book_roller, book_roller_event_handler, LV_EVENT_ALL, context);
}

static void button_1_clicked_cb(lv_event_t *e)
{
	LOG_DBG("Button 1 clicked with event");
	context_t *context = lv_event_get_user_data(e);
	LOG_DBG("Context: %s", context_strings[*context]);
	LOG_DBG("Event code: %d", lv_event_get_code(e));

	if (*context == READING)
	{
		zereader_print_prev_page();
		epub_write_current_book_state();
		// epub_get_current_book_state();
	}
	else if (*context == MENU)
	{
		uint32_t type = LV_KEY_UP;
		lv_obj_send_event(book_roller, LV_EVENT_KEY, &type);
	}
}

static void button_2_clicked_cb(lv_event_t *e)
{
	LOG_DBG("Button 2 clicked with event");
	context_t *context = lv_event_get_user_data(e);
	LOG_DBG("Context: %s", context_strings[*context]);

	if (*context == READING)
	{
		*context = MENU;

		lv_label_set_text(button_1_label, BT_UP);
		lv_label_set_text(button_2_label, BT_OK);
		lv_label_set_text(button_3_label, BT_EXIT);
		lv_label_set_text(button_4_label, BT_DOWN);

		zereader_show_bookmenu(context);
	}
	else if (*context == MENU)
	{
		uint32_t type = LV_KEY_ENTER;
		lv_obj_send_event(book_roller, LV_EVENT_VALUE_CHANGED, &type);
	}
}

static void button_3_clicked_cb(lv_event_t *e)
{
	LOG_DBG("Button 3 clicked with event");
	context_t *context = lv_event_get_user_data(e);
	LOG_DBG("Context: %s", context_strings[*context]);

	if (*context == READING)
	{
		// Nothing? Settings?
	}
	else if (*context == MENU)
	{
		*context = READING;
		lv_label_set_text(button_1_label, BT_PREV);
		lv_label_set_text(button_2_label, BT_MENU);
		lv_label_set_text(button_3_label, BT_NONE);
		lv_label_set_text(button_4_label, BT_NEXT);
		lv_obj_del(book_roller);
		book_roller = NULL;
		// lv_obj_invalidate(text_area);
	}
}

static void button_4_clicked_cb(lv_event_t *e)
{
	LOG_DBG("Button 4 clicked with event");
	context_t *context = lv_event_get_user_data(e);
	LOG_DBG("Context: %s", context_strings[*context]);

	if (*context == READING)
	{
		zereader_print_next_page();
		epub_write_current_book_state();
		// epub_get_current_book_state();
	}
	else if (*context == MENU)
	{
		uint32_t type = LV_KEY_DOWN;
		lv_obj_send_event(book_roller, LV_EVENT_KEY, &type);
		lv_timer_handler();
	}
}

void zereader_setup_control_buttons(context_t *context)
{
	button_1 = lv_button_create(lv_screen_active());
	lv_obj_add_style(button_1, &font_style, 0);
	lv_obj_align(button_1, LV_ALIGN_BOTTOM_LEFT, 10, -5);
	lv_obj_add_event_cb(button_1, button_1_clicked_cb, LV_EVENT_CLICKED, context);

	button_1_label = lv_label_create(button_1);
	lv_label_set_text(button_1_label, BT_PREV);
	lv_obj_center(button_1_label);

	button_2 = lv_button_create(lv_screen_active());
	lv_obj_add_style(button_2, &font_style, 0);
	lv_obj_align(button_2, LV_ALIGN_BOTTOM_MID, -50, -5);
	lv_obj_add_event_cb(button_2, button_2_clicked_cb, LV_EVENT_CLICKED, context);

	button_2_label = lv_label_create(button_2);
	lv_label_set_text(button_2_label, BT_MENU);
	lv_obj_center(button_2_label);

	button_3 = lv_button_create(lv_screen_active());
	lv_obj_add_style(button_3, &font_style, 0);
	lv_obj_align(button_3, LV_ALIGN_BOTTOM_MID, 50, -5);
	lv_obj_add_event_cb(button_3, button_3_clicked_cb, LV_EVENT_CLICKED, context);

	button_3_label = lv_label_create(button_3);
	lv_label_set_text(button_3_label, BT_NONE);
	lv_obj_center(button_3_label);

	button_4 = lv_button_create(lv_screen_active());
	lv_obj_add_style(button_4, &font_style, 0);
	lv_obj_align(button_4, LV_ALIGN_BOTTOM_RIGHT, -10, -5);
	lv_obj_add_event_cb(button_4, button_4_clicked_cb, LV_EVENT_CLICKED, context);

	button_4_label = lv_label_create(button_4);
	lv_label_set_text(button_4_label, BT_NEXT);
	lv_obj_center(button_4_label);
}

void zereader_setup_page()
{
	LOG_DBG("Setup page");
	lv_style_init(&font_style);
	lv_style_set_text_font(&font_style, &notoserif_14);

	text_area = lv_textarea_create(lv_screen_active());
	lv_obj_add_style(text_area, &font_style, 0);

	lv_obj_set_x(text_area, 10);
	lv_obj_set_y(text_area, 20);
	lv_obj_set_width(text_area, 780);
	lv_obj_set_height(text_area, 440);

	page_ctr = 0;
}

void screen_health()
{
	page_ctr++;

	if (page_ctr > 7)
	{
		page_ctr = 0;
		display_blanking_on(display_dev);
		lv_timer_handler();
		display_blanking_off(display_dev);
	}
}

void zereader_print_next_page()
{
	lv_textarea_set_text(text_area, epub_get_next_page());
	// lv_textarea_add_text(text_area, epub_get_next_page());
	screen_health();
}

void zereader_print_current_page()
{
	LOG_DBG("In print current page");
	epub_get_prev_page();
	LOG_DBG("Got prev page");
	char *page = epub_get_next_page();
	if (strcmp(page, "") == 0)
	{
		// Fetch the next page in case sitting on a chapter border
		page = epub_get_next_page();
	}

	lv_textarea_set_text(text_area, page);
	// lv_textarea_add_text(text_area, epub_get_next_page());
	screen_health();
}

void zereader_print_prev_page()
{
	lv_textarea_set_text(text_area, epub_get_prev_page());
	screen_health();
}

void zereader_clean_page()
{
	zereader_clean_logo();
	lv_textarea_set_text(text_area, " ");
	lv_timer_handler();
	display_blanking_on(display_dev);
	display_blanking_off(display_dev);
}

void zereader_show_logo()
{
	LV_IMG_DECLARE(zereaderlogomarx);
	logo = lv_image_create(lv_screen_active());
	lv_image_set_src(logo, &zereaderlogomarx);
	lv_obj_align(logo, LV_ALIGN_CENTER, 0, 30);
}

void zereader_clean_logo()
{
	if (logo != NULL)
	{
		lv_obj_del(logo);
		logo = NULL;
	}
}
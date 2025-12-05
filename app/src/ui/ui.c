/*
 * SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#include <epub/epub.h>

#include <stdio.h>
#include <string.h>

#include <ui/ui.h>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wbidi-chars"
#endif /* __GNUC__ */

#include <ui/fonts/notoserif_14.h>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif /* __GNUC__ */

#include <ui/logo/zereaderlogomarx.h>

#include <zephyr/logging/log.h>

#define BATTERY_UPDATE_INTERVAL_MS 10000 // Update every 10 seconds
#define BAT_MIN_MV 3300									 // 0% charge (3.3V)
#define BAT_MAX_MV 4200									 // 100% charge (4.2V)
#define BAT_CHARGER_MV 5000							 // Threshold to detect charger (approx 5V)

LOG_MODULE_REGISTER(ui, CONFIG_ZEREADER_LOG_LEVEL);
LV_FONT_DECLARE(notoserif_14);

const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

// Currently, only one ADC channel is defined in the device tree.
// Thus, the ADC channel 3, bound to 1/3 VSYS is indexed 0.
// static const struct adc_dt_spec
const struct adc_dt_spec adc_chan3_vsys = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

static lv_obj_t *bat_label;

static lv_timer_t *bat_timer;

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

static uint8_t get_battery_percentage(int32_t battery_mv)
{
	if (battery_mv >= BAT_MAX_MV)
		return 100;
	if (battery_mv <= BAT_MIN_MV)
		return 0;

	return (uint8_t)((battery_mv - BAT_MIN_MV) * 100 / (BAT_MAX_MV - BAT_MIN_MV));
}

static const char *get_battery_symbol(uint8_t percentage)
{
	if (percentage > 90)
		return LV_SYMBOL_BATTERY_FULL;
	if (percentage > 70)
		return LV_SYMBOL_BATTERY_3;
	if (percentage > 50)
		return LV_SYMBOL_BATTERY_2;
	if (percentage > 20)
		return LV_SYMBOL_BATTERY_1;

	return LV_SYMBOL_BATTERY_EMPTY;
}

static void update_battery_cb(lv_timer_t *timer)
{
	int err;
	int32_t val_mv;
	uint16_t buf;

	struct adc_sequence sequence = {
			.buffer = &buf,
			/* buffer size in bytes, not number of samples */
			.buffer_size = sizeof(buf),
	};

	(void)adc_sequence_init_dt(&adc_chan3_vsys, &sequence);

	err = adc_read_dt(&adc_chan3_vsys, &sequence);
	if (err < 0)
	{
		LOG_ERR("ADC 3 (VSYS) read failed: (%d)\n", err);
		return;
	}

	val_mv = (int32_t)buf;
	LOG_DBG("%" PRId32, val_mv);

	if (adc_raw_to_millivolts_dt(&adc_chan3_vsys, &val_mv) == 0)
	{
		val_mv = (val_mv * 3);
		LOG_DBG("ADC reading - %s, channel %d: raw: %" PRId32 " mV", adc_chan3_vsys.dev->name, adc_chan3_vsys.channel_id, val_mv);

		val_mv = val_mv - 50;
		val_mv /= 10;

		val_mv *= 10;
		LOG_DBG("Battery voltage rounded: %" PRId32 " mV", val_mv);
	}

	if (val_mv >= BAT_CHARGER_MV)
	{
		// Only works for the Pi Pico's USB connector.
		// Connect the BQ25xx's charging indicator in the next revision
		// for a proper charging detection.
		lv_label_set_text(bat_label, "USB Connected " LV_SYMBOL_CHARGE);
	}
	else
	{
		uint8_t percentage = get_battery_percentage(val_mv);
		lv_label_set_text_fmt(bat_label, "%d%% %s", percentage, get_battery_symbol(percentage));
	}
}

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
		snprintf(book_entry, 49, "%zu - %s - %s", books->book->number, books->book->author, books->book->title);
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

void zereader_setup_statusbar()
{
	bat_label = lv_label_create(lv_screen_active());
	lv_obj_align(bat_label, LV_ALIGN_TOP_RIGHT, -40, 5);
	lv_label_set_text(bat_label, "--% " LV_SYMBOL_BATTERY_EMPTY); // Initial State

	bat_timer = lv_timer_create(update_battery_cb, BATTERY_UPDATE_INTERVAL_MS, NULL);

	lv_timer_ready(bat_timer);
}

int zereader_initialize_peripherals()
{
	// Initialize the choosen zephyr,display device
	// -> Make the device tree description available for the software part
	if (!device_is_ready(display_dev))
	{
		LOG_ERR("Device not ready, aborting...");
		return -1;
	}

	// Make the FIRST ok zephyr,lvgl-button-input node available to the software part
	static const struct device *lvgl_btn_dev;
	lvgl_btn_dev = DEVICE_DT_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_lvgl_button_input));
	if (!device_is_ready(lvgl_btn_dev))
	{
		LOG_ERR("Device not ready, aborting...");
		return -2;
	}

	/* Configure channels individually prior to sampling. */
	if (!adc_is_ready_dt(&adc_chan3_vsys))
	{
		printk("ADC controller device %s not ready\n", adc_chan3_vsys.dev->name);
		return -3;
	}

	if (adc_channel_setup_dt(&adc_chan3_vsys) < 0)
	{
		printk("Could not setup channel #3 (ADC VSYS)\n");
		return -4;
	}

	return 0;
}

void zereader_setup_page()
{
	LOG_DBG("Setup page");

	lv_style_init(&font_style);
	lv_style_set_text_font(&font_style, &notoserif_14);

	text_area = lv_textarea_create(lv_screen_active());
	lv_obj_add_style(text_area, &font_style, 0);

	lv_obj_set_x(text_area, 10);
	lv_obj_set_y(text_area, 23);
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

void zereader_display_blanking_off()
{
	display_blanking_off(display_dev);
}

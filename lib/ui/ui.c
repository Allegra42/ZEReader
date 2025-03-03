#include <zephyr/logging/log.h>

#include <ui/ui.h>

LOG_MODULE_REGISTER(ui, CONFIG_DISPLAY_LOG_LEVEL);

lv_obj_t *button_left;
lv_obj_t *button_mid;
lv_obj_t *button_right;

lv_obj_t *button_left_label;
lv_obj_t *button_mid_label;
lv_obj_t *button_right_label;

lv_obj_t *page_label;

static void button_left_clicked_cb(lv_event_t *e)
{
	LOG_DBG("Left button clicked with event");
	context_t *context = lv_event_get_user_data(e);
	LOG_DBG("Context: %s", context_strings[*context]);
	LOG_DBG("Event code: %d", lv_event_get_code(e));
	zereader_print_page();
}

static void button_mid_clicked_cb(lv_event_t *e)
{
	LOG_DBG("Middle button clicked with event");
	context_t *context = lv_event_get_user_data(e);
	LOG_DBG("Context: %s", context_strings[*context]);
}

static void button_right_clicked_cb(lv_event_t *e)
{
	LOG_DBG("Right button clicked with event");
	context_t *context = lv_event_get_user_data(e);
	LOG_DBG("Context: %s", context_strings[*context]);
	zereader_clean_page();
}

void zereader_setup_control_buttons(context_t *context)
{
	// lv_display_set_rotation(lv_display_get_default(), LV_DISPLAY_ROTATION_90);
	// lv_theme_mono_init(lv_display_get_default(), false, LV_FONT_MONTSERRAT_48);
	// lv_theme_default_init(lv_display_get_default(), lv_color_white(), lv_color_black(), false, LV_FONT_MONTSERRAT_24);
	// lv_theme_apply(lv_screen_active());

	// lv_style_t style_default;
	// lv_style_init(&style_default);
	// lv_style_set_bg_color(&style_default, lv_color_black());
	// lv_style_set_border_width(&style_default, 2);
	// lv_style_set_text_color(&style_default, lv_color_white());
	// lv_style_set_border_color(&style_default, lv_color_white());

	// lv_obj_set_style_bg_color(lv_screen_active(), lv_color_white(), LV_PART_MAIN);
	lv_obj_set_style_text_color(lv_screen_active(), lv_color_white(), LV_PART_MAIN);

	LOG_DBG("Button Setup");
	button_left = lv_button_create(lv_screen_active());
	// lv_obj_add_style(button_left, &style_default, 0);
	LOG_DBG("Button created");
	lv_obj_align(button_left, LV_ALIGN_BOTTOM_LEFT, 10, -5);
	LOG_DBG("Button aligned");
	lv_obj_add_event_cb(button_left, button_left_clicked_cb, LV_EVENT_CLICKED, context);
	LOG_DBG("Button callback added");

	button_left_label = lv_label_create(button_left);
	lv_label_set_text(button_left_label, "Left Button");
	lv_obj_center(button_left_label);

	button_mid = lv_button_create(lv_screen_active());
	// lv_obj_add_style(button_mid, &style_default, 0);
	lv_obj_align(button_mid, LV_ALIGN_BOTTOM_MID, 0, -5);
	lv_obj_add_event_cb(button_mid, button_mid_clicked_cb, LV_EVENT_CLICKED, context);

	button_mid_label = lv_label_create(button_mid);
	lv_label_set_text(button_mid_label, "Middle Button");
	lv_obj_center(button_mid_label);

	button_right = lv_button_create(lv_screen_active());
	// lv_obj_add_style(button_right, &style_default, 0);
	lv_obj_align(button_right, LV_ALIGN_BOTTOM_RIGHT, -10, -5);
	lv_obj_add_event_cb(button_right, button_right_clicked_cb, LV_EVENT_CLICKED, context);

	button_right_label = lv_label_create(button_right);
	lv_label_set_text(button_right_label, "Right Button");
	lv_obj_center(button_right_label);
}

void zereader_setup_page()
{
	LOG_DBG("Setup page");
	page_label = lv_label_create(lv_screen_active());
	LOG_DBG("Created page label");
	lv_obj_align(page_label, LV_ALIGN_TOP_LEFT, 10, 20);
	LOG_DBG("Aligned page label");
}

void zereader_print_page()
{
	LOG_DBG("Print page");
	lv_label_set_text(page_label,
					  "This is\na test!\nLets print some demo texts.\nI'm Anna, I love squirrels!");
}

void zereader_clean_page()
{
	LOG_DBG("Clean page");
	lv_label_set_text(page_label, " ");
}
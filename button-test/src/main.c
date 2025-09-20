/*
 * SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#include <stdio.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(main, CONFIG_ZEREADER_LOG_LEVEL);

#define LONG_PRESS_TIMEOUT K_SECONDS(1)
struct k_timer long_press_timer;

#define PWR DT_ALIAS(pwrpin)
#define ONOFF DT_ALIAS(onoff)
static const struct gpio_dt_spec powerpin = GPIO_DT_SPEC_GET(PWR, gpios);
static const struct gpio_dt_spec onoff_button = GPIO_DT_SPEC_GET(ONOFF, gpios);
static struct gpio_callback button_cb_data;

// static void button_handler(uint32_t button_state, uint32_t has_changed)
// {
//     if (DK_BTN1_MSK & has_changed) {
//         if (DK_BTN1_MSK & button_state) {
//             /* Button changed its state to pressed */
//             k_timer_start(&long_press_timer, LONG_PRESS_TIMEOUT, K_NO_WAIT);
//         } else {
//             /* Button changed its state to released */
//             if (k_timer_status_get(&long_press_timer) > 0) {
//                 /* Timer expired before button was released, indicates long press */
//                 LOG_INF("Long press");
//             } else {
//                 LOG_INF("Short press");
//                 k_timer_stop(&long_press_timer);
//             }
//         }
//     }
// }

void onoff_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	LOG_DBG("Button pressed!");

}

int main(void)
{
	int ret = 0;

	LOG_DBG("Hello World - ZEReader! %s\n", CONFIG_BOARD_TARGET);

	if (!gpio_is_ready_dt(&powerpin))
	{
		LOG_DBG("Failed 1");
		return 0;
	}
	LOG_DBG("Power Pin ready!");

	ret = gpio_pin_configure_dt(&powerpin, GPIO_OUTPUT_LOW);
	if (ret < 0)
	{
		LOG_DBG("Failed 3");
		return 0;
	}
	LOG_DBG("Power pin configured");
	// LOG_DBG("Alive");
	// gpio_pin_set_dt(&powerpin, 1);

	if (!gpio_is_ready_dt(&onoff_button))
	{
		printk("Error: button device %s is not ready\n",
		       onoff_button.port->name);
		LOG_DBG("Failed 2");
		return 0;
	}
	LOG_DBG("Button ready!");
	ret = gpio_pin_configure_dt(&onoff_button, GPIO_INPUT);
	if (ret < 0)
	{
		LOG_DBG("Failed 4");
		return 0;
	}
	LOG_DBG("Button configured");
	ret = gpio_pin_interrupt_configure_dt(&onoff_button, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0)
	{
		LOG_DBG("Failed 5");
		return 0;
	}

	gpio_init_callback(&button_cb_data, onoff_pressed, BIT(onoff_button.pin));
	gpio_add_callback(onoff_button.port, &button_cb_data);

	LOG_DBG("Interrupt configured");

	k_msleep(2000);
	gpio_pin_set_dt(&powerpin, 1);

	

	// k_msleep(5000);
	// LOG_DBG("Turn off");
	// gpio_pin_set_dt(&powerpin, 0);
	while (1) {
		k_msleep(500);
		LOG_DBG("x");
	}
	
}
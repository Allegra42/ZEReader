/*
 * SPDX-FileCopyrightText: 2026 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef DEVICE_MANAGEMENT_H
#define DEVICE_MANAGEMENT_H

/**
 * @file
 * @brief The ZEReader Device Management.
 * @defgroup device_management Device Initialization and managment
 * @ingroup dev_mgmt
 * @{
 */

#define DEV_MGMT_SUCCESS 0
#define DEV_MGMT_ERROR_DISPLAY_NOT_READY -1
#define DEV_MGMT_ERROR_LVGL_INPUT_NOT_READY -2
#define DEV_MGMT_ERROR_ADC_NOT_READY -3

/**
 * @brief Initialize peripheral devices defined in the device tree.
 *
 * @retval 0 on success.
 * @retval negative on error.
 */
int dev_mgmt_init(void);

/**
 * @brief Turn on display blanking
 */
void dev_mgmt_display_blanking_on(void);

/**
 * @brief Turn off display blanking
 */
void dev_mgmt_display_blanking_off(void);

/** @} */
#endif /* DEVICE_MANAGEMENT_H */
/*
 * SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef _SD_H_
#define _SD_H_

#include <stddef.h>
#include <zephyr/fs/fs.h>

/**
 * @file
 * @brief The ZEReader SD card library.
 * @defgroup sd_filemanagement SD card file management
 * @ingroup sd
 * @{
 */

#define SD_MOUNTPOINT "/SD:"
#define SD_ROOTPATH "/SD:/"

/**
 * @brief Returns a char pointer on the fully built-up SD card path.
 *
 * @param[in] folder The folder's name
 * @param[in] filename The file's name
 * @param[in, out] error An int-typed error value
 *
 * @retval 0 on success.
 * @retval -ENAMETOOLONG If the path length is longer than CONFIG_FS_FATFS_MAX_LFN (255).
 * @retval -ENOMEM If there is not enough RAM left to allocate memory for the full path.
 */
char *sd_build_full_path(const char *folder, const char *filename, int *error);

/**
 * @brief Initializes the SD card communication.
 *
 * @retval -ENODEV If the device was not found.
 * @retval <0 If getting the sector count or size failed or the card could not get mounted.
 */
int sd_initialize(void);

/**
 * @brief Open a file on a SD card
 *
 * @param[in] path The file's full path.
 * @param[in, out] f_obj A pointer to the file object representing the file.
 *
 * @retval 0 on success.
 */
int sd_open(char const *const path, struct fs_file_t *f_obj);

/**
 * @brief Close a file on a SD card.
 *
 * @param[in] f_obj A pointer to the file object representing the file.
 *
 * @retval 0 on success.
 */
int sd_close(struct fs_file_t *f_obj);

/**
 * @brief Read from a file on a SD card.
 *
 * @param[in, out] f_obj A file object representing the file.
 * @param[in, out] buffer A pointer to the data buffer.
 * @param[in, out] size The number of bytes to read.
 *
 * @retval >=0 a number of bytes read, on success.
 */
int sd_read(struct fs_file_t *f_obj, char *buffer, size_t *size);

/**
 * @brief Open, read from a file on SD card and close it.
 *
 * @param[in] path The name of the file to read from.
 * @param[in, out] offset Relative location to move the file pointer to.
 * @param[in, out] buffer A pointer to the data buffer.
 * @param[in, out] size The number of bytes to read.
 *
 * @retval >=0 a number of bytes read, on success.
 */
int sd_read_chunk(char const *const path, size_t *offset, char *const buffer, size_t *size);

// int sd_write();

/**
 * @brief Open, write to a file on SD card and close it.
 *
 * @param[in] path The name of the file to write to.
 * @param[in] data A pointer to the data buffer to be written.
 * @param[in, out] size The number of bytes to be written.
 *
 * @retval 0 on success.
 */
int sd_write_chunk(char const *const path, char const *const data, size_t *size);

/**
 * @brief List the directories on the SD card.
 *
 * @param[in] path The path on the SD card to be listed.
 * @param[in, out] buffer A pointer to the data buffer to be written.
 * @param[in, out] size The maximum number of bytes to be written.
 *
 * @retval >=0 the number of bytes written, on success.
 */
int sd_list_directories(char const *const path, char *buffer, size_t *size);

/**
 * @brief Get the current location of the file pointer inside the file.
 *
 * @param[in] path The name of the file.
 * @param[in, out] offset The offset within the file.
 *
 * @retval 0 on success.
 */
int sd_tell_end_offset(char const *const path, size_t *offset);

/** @} */
#endif
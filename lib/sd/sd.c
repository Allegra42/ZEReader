/*
 * SPDX-FileCopyrightText: 2025 Anna-Lena Marx <mail@marx.engineer>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/fs/fs.h>
#include <ff.h>
#include <string.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>

#include <sd/sd.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sd_utils, CONFIG_ZEREADER_LOG_LEVEL);

static FATFS fat_fs;
K_MUTEX_DEFINE(sd_fs_mutex);

static struct fs_mount_t mount_point = {
    .type = FS_FATFS,
    .fs_data = &fat_fs,
};

char *sd_build_full_path(const char *folder, const char *filename, int *error)
{
    char *fullpath = NULL;
    uint32_t path_len = strlen(SD_ROOTPATH) + strlen(folder) + strlen(filename);

    if (path_len > CONFIG_FS_FATFS_MAX_LFN)
    {
        LOG_ERR("Filename too long!");
        *error = -ENAMETOOLONG;
        return NULL;
    }

    fullpath = (char *)malloc(path_len + 1);
    if (fullpath == NULL)
    {
        LOG_ERR("Could not allocate any memory!");
        *error = -ENOMEM;
        return NULL;
    }

    memset(fullpath, 0, path_len + 1);
    memcpy(fullpath, SD_ROOTPATH, strlen(SD_ROOTPATH));
    strncat(fullpath, folder, strlen(folder));
    strncat(fullpath, filename, strlen(filename));

    LOG_DBG("Full path: %s", fullpath);
    *error = 0;
    return fullpath;
}

int sd_initialize(void)
{
    int ret;
    static const char *sd_dev = "SD";
    uint32_t sector_count;
    uint32_t sector_size;

    ret = disk_access_ioctl(sd_dev, DISK_IOCTL_CTRL_INIT, NULL);
    if (ret)
    {
        LOG_DBG("Init failed, is SD card inserted?");
        return -ENODEV;
    }

    ret = disk_access_ioctl(sd_dev, DISK_IOCTL_GET_SECTOR_COUNT, &sector_count);
    if (ret)
    {
        LOG_DBG("Cannot get sector count");
        return ret;
    }

    ret = disk_access_ioctl(sd_dev, DISK_IOCTL_GET_SECTOR_SIZE, &sector_size);
    if (ret)
    {
        LOG_DBG("Cannot get sector size");
        return ret;
    }

    mount_point.mnt_point = SD_MOUNTPOINT;

    ret = fs_mount(&mount_point);
    if (ret)
    {
        LOG_ERR("Mounting SD card failed, is the card FAT formatted?");
        return ret;
    }

    LOG_DBG("SD card initialized!");
    return 0;
}

int sd_open(char const *const path, struct fs_file_t *f_obj)
{
    k_mutex_lock(&sd_fs_mutex, K_FOREVER);
    int ret;

    fs_file_t_init(f_obj);

    ret = fs_open(f_obj, path, FS_O_READ);
    if (ret)
    {
        LOG_ERR("Could not open file: %d", ret);
    }
    k_mutex_unlock(&sd_fs_mutex);
    return ret;
}

int sd_close(struct fs_file_t *f_obj)
{
    k_mutex_lock(&sd_fs_mutex, K_FOREVER);
    int ret;

    ret = fs_close(f_obj);
    if (ret)
    {
        LOG_ERR("Could not close file: %d", ret);
    }
    k_mutex_unlock(&sd_fs_mutex);
    return ret;
}

int sd_read(struct fs_file_t *f_obj, char *buffer, size_t *size)
{
    k_mutex_lock(&sd_fs_mutex, K_FOREVER);
    int ret;

    ret = fs_read(f_obj, buffer, *size);
    if (ret < 0)
    {
        LOG_ERR("Could not read file: %d", ret);
        k_mutex_unlock(&sd_fs_mutex);
        return ret;
    }

    *size = ret;
    k_mutex_unlock(&sd_fs_mutex);
    return 0;
}

int sd_read_chunk(char const *const path, size_t *offset, char *const buffer, size_t *size)
{
    k_mutex_lock(&sd_fs_mutex, K_FOREVER);
    int ret;
    struct fs_file_t f_obj;

    fs_file_t_init(&f_obj);

    ret = fs_open(&f_obj, path, FS_O_READ);
    if (ret)
    {
        LOG_ERR("Could not open file: %d", ret);
        k_mutex_unlock(&sd_fs_mutex);
        return ret;
    }

    ret = fs_seek(&f_obj, *offset, FS_SEEK_SET);
    if (ret)
    {
        LOG_ERR("Could not seek file!");
        fs_close(&f_obj); // Close file on error before unlocking
        k_mutex_unlock(&sd_fs_mutex);
        return ret;
    }

    ret = fs_read(&f_obj, buffer, *size);
    if (ret < 0)
    {
        LOG_ERR("Could not read file: %d", ret);
        fs_close(&f_obj); // Close file on error before unlocking
        k_mutex_unlock(&sd_fs_mutex);
        return ret;
    }

    *size = ret;
    if (*size == 0)
    {
        LOG_DBG("File empty!");
    }

    *offset = fs_tell(&f_obj);

    ret = fs_close(&f_obj);
    if (ret)
    {
        LOG_ERR("Could not close file!");
    }
    k_mutex_unlock(&sd_fs_mutex);
    return ret;
}

int sd_tell_end_offset(char const *const path, size_t *offset)
{
    k_mutex_lock(&sd_fs_mutex, K_FOREVER);
    int ret;
    struct fs_file_t f_obj;
    fs_file_t_init(&f_obj);

    *offset = 0;

    ret = fs_open(&f_obj, path, FS_O_READ);
    if (ret)
    {
        LOG_ERR("Could not open file: %d", ret);
        k_mutex_unlock(&sd_fs_mutex);
        return ret;
    }

    ret = fs_seek(&f_obj, *offset, FS_SEEK_END);
    if (ret)
    {
        LOG_ERR("Could not seek file!");
        fs_close(&f_obj); // Close file on error before unlocking
        k_mutex_unlock(&sd_fs_mutex);
        return ret;
    }

    *offset = fs_tell(&f_obj);

    ret = fs_close(&f_obj);
    if (ret)
    {
        LOG_ERR("Could not close file!");
    }
    k_mutex_unlock(&sd_fs_mutex);
    return 0;
}

char *sd_read_whole_file(char const *const path, size_t *file_size)
{
    k_mutex_lock(&sd_fs_mutex, K_FOREVER);
    char *buffer = NULL;
    struct fs_file_t f_obj;
    int ret;

    *file_size = 0; // Initialize file_size to 0

    // Get file size
    ret = sd_get_file_size(path, file_size);
    if (ret != 0) {
        LOG_ERR("Failed to get file size for %s: %d", path, ret);
        k_mutex_unlock(&sd_fs_mutex);
        return NULL;
    }

    if (*file_size == 0) {
        LOG_DBG("File %s is empty, returning empty buffer.", path);
        buffer = (char *)malloc(1); // Return an empty string
        if (buffer) {
            buffer[0] = '\0';
        }
        k_mutex_unlock(&sd_fs_mutex);
        return buffer;
    }

    // Allocate memory for the file content + null terminator
    buffer = (char *)malloc(*file_size + 1);
    if (buffer == NULL) {
        LOG_ERR("Failed to allocate %zu bytes for file %s", *file_size + 1, path);
        k_mutex_unlock(&sd_fs_mutex);
        return NULL;
    }

    // Open file
    fs_file_t_init(&f_obj);
    ret = fs_open(&f_obj, path, FS_O_READ);
    if (ret != 0) {
        LOG_ERR("Failed to open file %s: %d", path, ret);
        free(buffer);
        k_mutex_unlock(&sd_fs_mutex);
        return NULL;
    }

    // Read whole file
    size_t bytes_read = *file_size;
    ret = fs_read(&f_obj, buffer, bytes_read);
    if (ret < 0) {
        LOG_ERR("Failed to read file %s: %d", path, ret);
        fs_close(&f_obj);
        free(buffer);
        k_mutex_unlock(&sd_fs_mutex);
        return NULL;
    }
    else if (ret != bytes_read) {
        LOG_WRN("Read %d bytes from %s, expected %zu", ret, path, bytes_read);
    }
    buffer[*file_size] = '\0'; // Null-terminate the buffer

    // Close file
    fs_close(&f_obj);
    k_mutex_unlock(&sd_fs_mutex);
    return buffer;
}

int sd_get_file_size(char const *const path, size_t *file_size)
{
    struct fs_dirent entry;
    int ret = fs_stat(path, &entry);
    if (ret == 0) {
        *file_size = entry.size;
    } else {
        LOG_ERR("Could not get file size for %s: %d", path, ret);
    }
    return ret;
}

// int sd_write();
int sd_write_chunk(char const *const path, char const *const data, size_t *size)
{
    k_mutex_lock(&sd_fs_mutex, K_FOREVER);
    int ret;
    struct fs_file_t f_obj;
    fs_file_t_init(&f_obj);

    ret = fs_open(&f_obj, path, FS_O_CREATE | FS_O_WRITE); //| FS_O_APPEND
    if (ret)
    {
        LOG_ERR("Could not create/open file: %d", ret);
        k_mutex_unlock(&sd_fs_mutex);
        return ret;
    }

    // Overwrite file!
    ret = fs_seek(&f_obj, 0, FS_SEEK_SET);
    if (ret)
    {
        LOG_ERR("Seek file pointer failed");
        fs_close(&f_obj); // Close file on error before unlocking
        k_mutex_unlock(&sd_fs_mutex);
        return ret;
    }

    ret = fs_write(&f_obj, data, *size);
    if (ret < 0)
    {
        LOG_ERR("Could not write file : %d", ret);
        fs_close(&f_obj); // Close file on error before unlocking
        k_mutex_unlock(&sd_fs_mutex);
        return ret;
    }

    *size = ret;

    ret = fs_close(&f_obj);
    if (ret)
    {
        LOG_ERR("Could not close file!");
    }
    k_mutex_unlock(&sd_fs_mutex);
    return ret;
}

int sd_list_directories(char const *const path, char *buffer, size_t *buffer_size)
{
    k_mutex_lock(&sd_fs_mutex, K_FOREVER);
    int ret;

    struct fs_dir_t dir_obj;
    struct fs_dirent entry;
    size_t used = 0;

    fs_dir_t_init(&dir_obj);

    ret = fs_opendir(&dir_obj, path);
    if (ret)
    {
        LOG_ERR("Open directory %s failed!", path);
        k_mutex_unlock(&sd_fs_mutex);
        return ret;
    }

    while (true)
    {
        ret = fs_readdir(&dir_obj, &entry);
        if (ret)
        {
            LOG_DBG("Could not read directory");
            fs_closedir(&dir_obj); // Close directory on error before unlocking
            k_mutex_unlock(&sd_fs_mutex);
            return ret;
        }

        if (entry.name[0] == 0)
        {
            break;
        }

        if (buffer != NULL)
        {
            size_t remaining = *buffer_size - used;
            ssize_t len;

            if (entry.type == FS_DIR_ENTRY_DIR)
            {
                len = snprintk(&buffer[used], remaining, "%s\n", entry.name);

                if (len >= remaining)
                {
                    LOG_ERR("Could not append to buffer: %d", len);
                    fs_closedir(&dir_obj); // Close directory on error before unlocking
                    k_mutex_unlock(&sd_fs_mutex);
                    return -EINVAL;
                }

                used += len;
            }
        }
        LOG_DBG("[%s] %s", entry.type == FS_DIR_ENTRY_DIR ? "DIR " : "FILE", entry.name);
    }

    ret = fs_closedir(&dir_obj);
    if (ret)
    {
        LOG_ERR("Could not close directory %s", path);
    }

    *buffer_size = used;
    k_mutex_unlock(&sd_fs_mutex);
    return ret;
}

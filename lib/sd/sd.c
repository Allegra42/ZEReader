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
    int ret;

    fs_file_t_init(f_obj);

    ret = fs_open(f_obj, path, FS_O_READ);
    if (ret)
    {
        LOG_ERR("Could not open file: %d", ret);
        return ret;
    }

    return 0;
}

int sd_close(struct fs_file_t *f_obj)
{
    int ret;

    ret = fs_close(f_obj);
    if (ret)
    {
        LOG_ERR("Could not close file: %d", ret);
        return ret;
    }

    return 0;
}

int sd_read(struct fs_file_t *f_obj, char *buffer, size_t *size)
{
    int ret;

    ret = fs_read(f_obj, buffer, *size);
    if (ret < 0)
    {
        LOG_ERR("Could not read file: %d", ret);
        return ret;
    }

    *size = ret;
    return 0;
}

int sd_read_chunk(char const *const path, size_t *offset, char *const buffer, size_t *size)
{
    int ret;
    struct fs_file_t f_obj;

    fs_file_t_init(&f_obj);

    ret = fs_open(&f_obj, path, FS_O_READ);
    if (ret)
    {
        LOG_ERR("Could not open file: %d", ret);
        return ret;
    }

    ret = fs_seek(&f_obj, *offset, FS_SEEK_SET);
    if (ret)
    {
        LOG_ERR("Could not seek file!");
        return ret;
    }

    ret = fs_read(&f_obj, buffer, *size);
    if (ret < 0)
    {
        LOG_ERR("Could not read file: %d", ret);
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
        return ret;
    }

    return 0;
}

int sd_tell_end_offset(char const *const path, size_t *offset)
{
    int ret;
    struct fs_file_t f_obj;
    fs_file_t_init(&f_obj);

    *offset = 0;

    ret = fs_open(&f_obj, path, FS_O_READ);
    if (ret)
    {
        LOG_ERR("Could not open file: %d", ret);
        return ret;
    }

    ret = fs_seek(&f_obj, *offset, FS_SEEK_END);
    if (ret)
    {
        LOG_ERR("Could not seek file!");
        return ret;
    }

    *offset = fs_tell(&f_obj);

    ret = fs_close(&f_obj);
    if (ret)
    {
        LOG_ERR("Could not close file!");
        return ret;
    }

    return 0;
}

// int sd_write();
int sd_write_chunk(char const *const path, char const *const data, size_t *size)
{
    int ret;
    struct fs_file_t f_obj;
    fs_file_t_init(&f_obj);

    ret = fs_open(&f_obj, path, FS_O_CREATE | FS_O_WRITE); //| FS_O_APPEND
    if (ret)
    {
        LOG_ERR("Could not create/open file: %d", ret);
        return ret;
    }

    // Overwrite file!
    ret = fs_seek(&f_obj, 0, FS_SEEK_SET);
    if (ret)
    {
        LOG_ERR("Seek file pointer failed");
        return ret;
    }

    ret = fs_write(&f_obj, data, *size);
    if (ret < 0)
    {
        LOG_ERR("Could not write file : %d", ret);
        return ret;
    }

    *size = ret;

    ret = fs_close(&f_obj);
    if (ret)
    {
        LOG_ERR("Could not close file!");
        return ret;
    }

    return 0;
}

int sd_list_directories(char const *const path, char *buffer, size_t *buffer_size)
{
    int ret;

    struct fs_dir_t dir_obj;
    static struct fs_dirent entry;
    size_t used = 0;

    fs_dir_t_init(&dir_obj);

    ret = fs_opendir(&dir_obj, path);
    if (ret)
    {
        LOG_ERR("Open directory %s failed!", path);
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
        return ret;
    }

    *buffer_size = used;
    return 0;
}

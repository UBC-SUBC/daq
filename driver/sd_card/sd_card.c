#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/fs/fs.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <ff.h>

#include "sd_card.h"

static FATFS fat_fs;
static struct fs_file_t file;
static struct fs_mount_t sd_mount = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
	.mnt_point = DISK_MOUNT_PT,
};


static int create_new_file(const char *file_path, struct fs_file_t* file, struct tm* t)
{
	if (file_path == NULL || file == NULL) {
		return 1; // error, invalid arguments
	}
	
	// definitions
	char final_path_buffer[PATH_LENGTH];
	int file_path_len = strlen(file_path);
	int sd_card_path_len = strlen(DISK_MOUNT_PT); // find the length of "/SD"


	// file path length check
	if (file_path_len + sd_card_path_len + 20 > PATH_LENGTH) {
		return 1; 			// error, 1 indicates that the path is too long
	}

	strcpy(final_path_buffer, DISK_MOUNT_PT);

	final_path_buffer[sd_card_path_len] = '/';
	final_path_buffer[sd_card_path_len + 1] = '\0';
 
	// Extract the filename from file_path
	const char *filename = strrchr(file_path, '/');
	if (filename) {
		filename++; // Move past the last '/'
	} else {
		filename = file_path; // No '/' found, means no folder sturcutre, just file name
	}

	int final_path_buffer_length = strlen(final_path_buffer);

	// snprintf(destination, max_len, format, args...) writes a formatted string and null terminates if space allows.
	if (t == NULL) {
		snprintf(&final_path_buffer[final_path_buffer_length], PATH_LENGTH - final_path_buffer_length, "%s", file_path);
	} 
	else {
		snprintf(&final_path_buffer[final_path_buffer_length], PATH_LENGTH - final_path_buffer_length, "%04d-%02d-%02d-%s",
			 t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, filename);
	}

	fs_file_t_init(file);

	if (fs_open(file, final_path_buffer, FS_O_CREATE | FS_O_WRITE) != 0) {
		return 1; 
	} 
	return 0; 
}

int sd_card_init(void)
{
	int ret = disk_access_ioctl(DISK_DRIVE_NAME, DISK_IOCTL_CTRL_INIT, NULL);

	if (ret != 0) {
		printk("SD: disk init failed: %d\n", ret);
		return ret;
	}

	ret = fs_mount(&sd_mount);
	if (ret != 0) {
		printk("SD: mount failed: %d\n", ret);
		return ret;
	}

	ret = create_new_file(DEFAUTL_FILE_NAME, &file, NULL); // create a test file to check if the SD card is working properly
	ret = add_data_to_file(file_header, strlen(file_header));
	return ret;
}

int sd_card_cleanup(void)
{
	int ret = 0;
	int close_ret;

	close_ret = close_file(&file);
	if (close_ret != 0) {
		printk("SD: close failed: %d\n", close_ret);
		ret = close_ret;
	}

	close_ret = fs_unmount(&sd_mount);
	if (close_ret != 0) {
		printk("SD: unmount failed: %d\n", close_ret);
		ret = close_ret;
	}

	return ret;
}

int add_data_to_file(const char *data, size_t data_len)
{
	if (fs_write(&file, data, data_len) < 0 ) {
		return 1;
	}

	return 0;

}

int close_file(struct fs_file_t* file)
{
	if (fs_close(file) != 0) {
		return 1; 
	}
	return 0; 
}


// haven't go through
int lsdir(const char *path)
{
	int res;
	struct fs_dir_t dirp;
	static struct fs_dirent entry;
	int count = 0;

	fs_dir_t_init(&dirp);

	/* Verify fs_opendir() */
	res = fs_opendir(&dirp, path);
	if (res) {
		printk("Error opening dir %s [%d]\n", path, res);
		return 1;
	}

	printk("\nListing dir %s ...\n", path);
	for (;;) {
		/* Verify fs_readdir() */
		res = fs_readdir(&dirp, &entry);

		/* entry.name[0] == 0 means end-of-dir */
		if (res || entry.name[0] == 0) {
			break;
		}

		if (entry.type == FS_DIR_ENTRY_DIR) {
			printk("[DIR ] %s\n", entry.name);
		} else {
			printk("[FILE] %s (size = %zu)\n",
				entry.name, entry.size);
		}
		count++;
	}

	/* Verify fs_closedir() */
	fs_closedir(&dirp);
	if (res == 0) {
		res = count;
	}

	return res == 0 ? 0 : 1;
}

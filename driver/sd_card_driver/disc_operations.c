#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/fs/fs.h>
#include <ff.h>
#include <string.h>

#include "disc_operations.h"

char path_buffer[PATH_LENGTH];
static FATFS fat_fs;
static struct fs_file_t data_file;
static struct fs_mount_t sd_mount = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
	.mnt_point = DISK_MOUNT_PT,
};
struct fs_file_t* file_handle = &data_file;

/*
 * Build a full SD-card path from the mount point and base path, initialize the
 * file handle, and create/open the file.
 */
static int create_new_file(const char *base_path, struct fs_file_t* file)
{
	if (base_path == NULL || file == NULL) {
		return -1; // error, invalid arguments
	}
	
	// definitions
	int base_length = strlen(base_path);
	int sd_length = strlen(DISK_MOUNT_PT);

	if (base_length + sd_length + 2 > PATH_LENGTH) {
		return 1; 			// error, 1 indicates that the path is too long
	}

	// start building the full path
	strcpy(path_buffer, DISK_MOUNT_PT);

	path_buffer[sd_length] = '/';
	path_buffer[sd_length + 1] = '\0';

	strncat(path_buffer, base_path, PATH_LENGTH - strlen(path_buffer) - 1);

	fs_file_t_init(file);

	if (fs_open(file, path_buffer, FS_O_CREATE | FS_O_WRITE | FS_O_APPEND) != 0) {
		return -1; 
	} 
	return 0; 
}

void init_sd_card(void)
{
	if (disk_access_init(DISK_DRIVE_NAME) != 0) {
		return;
	}

	if (fs_mount(&sd_mount) != 0) {
		return;
	}

	create_new_file("data.txt", file_handle);
}


int add_data_to_file(struct fs_file_t* file, const char *data, size_t data_len)
{
	ssize_t bytes_written = fs_write(file, data, data_len);

	if (bytes_written < 0 || (size_t)bytes_written != data_len) {
		return -1;
	}

	return 0;
}

int close_file(struct fs_file_t* file)
{
	if (fs_close(file) != 0) {
		return -1; 
	}
	return 0; 
}

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
		return res;
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

	return res;
}

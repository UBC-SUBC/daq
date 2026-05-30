#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/fs/fs.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "sd_card.h"

int create_new_file(const char *base_path, struct fs_file_t* file, struct tm* t)
{
	if (base_path == NULL || file == NULL) {
		return -1; // error, invalid arguments
	}
	
	// definitions
	char path_buffer[PATH_LENGTH];
	int base_length = strlen(base_path);
	int sd_length = strlen(DISK_MOUNT_PT);

	if (base_length + sd_length + 20 > PATH_LENGTH) { // Adjusted for date and separator
		return 1; 			// error, 1 indicates that the path is too long
	}

	strcpy(path_buffer, DISK_MOUNT_PT);

	path_buffer[sd_length] = '/';
	path_buffer[sd_length + 1] = '\0';
 
	// Extract the filename from base_path
	const char *filename = strrchr(base_path, '/');
	if (filename) {
		filename++; // Move past the last '/'
	} else {
		filename = base_path; // No '/' found, use the entire base_path
	}

	int path_buffer_length = strlen(path_buffer);

	if (t == NULL) {
		snprintf(&path_buffer[path_buffer_length], PATH_LENGTH - path_buffer_length, "%s", base_path);
	} else {
		snprintf(&path_buffer[path_buffer_length], PATH_LENGTH - path_buffer_length, "%04d-%02d-%02d-%s",
			 t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, filename);
	}

	fs_file_t_init(file);

	if (fs_open(file, path_buffer, FS_O_CREATE | FS_O_WRITE) != 0) {
		return -1; 
	} 
	return 0; 
}

int add_data_to_file(struct fs_file_t* file, const char *data, size_t data_len)
{
	if (fs_write(file, data, data_len) < 0 ) {
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

#ifndef DISC_OPERATIONS_H
#define DISC_OPERATIONS_H

#include <stddef.h>
#include <zephyr/fs/fs.h>

/*
* SD card configuration:
*/
#define DISK_DRIVE_NAME "SD"
#define DISK_MOUNT_PT "/"DISK_DRIVE_NAME":"
#define PATH_LENGTH 256

// extern char path_buffer[PATH_LENGTH];
// extern struct fs_file_t* file_handle;

// Function prototypes:
void init_sd_card(void);
int lsdir(const char *path);
int add_data_to_file(struct fs_file_t* file, const char *data, size_t data_len);
int close_file(struct fs_file_t* file);

#endif /* DISC_OPERATIONS_H */

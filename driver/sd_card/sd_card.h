#ifndef SD_CARD_H
#define SD_CARD_H

#include <zephyr/fs/fs.h>
#include <stdint.h>
#include <stddef.h>
#include <time.h>


#define DISK_DRIVE_NAME "SD"
#define DISK_MOUNT_PT "/"DISK_DRIVE_NAME":"
#define PATH_LENGTH 256
#define DEFAUTL_FILE_NAME "data.txt"

int sd_card_init(void);
int sd_card_cleanup(void);
int lsdir(const char *path);
int close_file(struct fs_file_t* file);

typedef struct {
    uint8_t temp;
    uint8_t pressure;
    uint8_t rpm;
    uint8_t xxx;
    uint8_t yyy;
    uint8_t ccc;
}sd_data_struct;

int add_sensor_data_to_file(sd_data_struct* data);

static const char file_header[] = "temp,pressure,rmp,xxx,yyy,zzz";

#endif /* SD_CARD_H */

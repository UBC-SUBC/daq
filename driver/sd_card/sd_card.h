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
    uint16_t humidity;
    uint8_t pressure;
    uint8_t rpm;
    uint8_t x;
    uint8_t y;
    uint8_t z;
}sd_data_struct;

int add_sensor_data_to_file(sd_data_struct* data);

static const char file_header[] = "time(ms),temp(C),hum(%%),pres,rpm,x,y,z";

#endif /* SD_CARD_H */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/fs/fs.h>
#include <zephyr/storage/disk_access.h>
#include <errno.h>
#include <string.h>
#include <ff.h>

#include "sd_card.h"

#define SD_SPI_NODE DT_NODELABEL(spi1)

static const struct device *const sd_spi = DEVICE_DT_GET(SD_SPI_NODE);
static FATFS fat_fs;
static struct fs_mount_t sd_mount = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
	.mnt_point = DISK_MOUNT_PT,
};

/* SPI1 default pins are configured by Zephyr from the board devicetree. */
static int sd_card_spi_init(void)
{
	if (!device_is_ready(sd_spi)) {
		return -ENODEV;
	}

	return 0;
}

static int sd_card_mount(void)
{
	int ret = disk_access_ioctl(DISK_DRIVE_NAME, DISK_IOCTL_CTRL_INIT, NULL);

	if (ret != 0) {
		return ret;
	}

	return fs_mount(&sd_mount);
}

int main(void)
{
	static const char data[] = "SD card write test\r\n";
	struct fs_file_t file;
	int ret;

	if (sd_card_spi_init() != 0) {
		return -ENODEV;
	}

	ret = sd_card_mount();
	if (ret != 0) {
		return ret;
	}

	ret = create_new_file("daq.txt", &file, NULL);
	if (ret != 0) {
		fs_unmount(&sd_mount);
		return ret;
	}

	ret = add_data_to_file(&file, data, strlen(data));
	close_file(&file);
	fs_unmount(&sd_mount);

	return ret;
}

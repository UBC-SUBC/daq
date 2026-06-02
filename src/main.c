#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/fs/fs.h>
#include <zephyr/storage/disk_access.h>
#include <errno.h>
#include <string.h>
#include <ff.h>

#include "sd_card.h"

#define SD_SPI_NODE DT_NODELABEL(spi1)

// ========== Configuration ==========
#define DEBUG 1
//====================================

static const struct device *const sd_spi = DEVICE_DT_GET(SD_SPI_NODE);
static const struct gpio_dt_spec sd_cs = GPIO_DT_SPEC_GET_BY_IDX(SD_SPI_NODE, cs_gpios, 0);

static int sd_card_spi_init(void)
{
	if (!device_is_ready(sd_spi)) {
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&sd_cs)) {
		return -ENODEV;
	}

	return 0;
}

int main(void)
{
	// static const char data[] = "SD card write test\r\n";
	int ret;
	int close_ret;

	ret = sd_card_spi_init();
	
	#if DEBUG
		if (ret != 0) {
			printk("SD: SPI1 or CS not ready: %d\n", ret);
			return ret;
		}
	#else
		if (ret != 0) {
			return ret;
		}
	#endif

	ret = sd_card_mount();
	if (ret != 0) {
		return ret;
	}
	printk("SD: mounted at %s\n", DISK_MOUNT_PT);

	// ret = create_new_file("daq.txt", &file, NULL);
	// if (ret != 0) {
	// 	printk("SD: create file failed: %d\n", ret);
	// 	fs_unmount(&sd_mount);
	// 	return ret;
	// }
	// printk("SD: created daq.txt\n");

	// ret = add_data_to_file(&file, data, strlen(data));
	// if (ret != 0) {
	// 	printk("SD: write failed: %d\n", ret);
	// } else {
	// 	printk("SD: wrote %u bytes\n", (unsigned int)strlen(data));
	// }

	// close_ret = close_file(&file);
	// if (close_ret != 0) {
	// 	printk("SD: close failed: %d\n", close_ret);
	// 	ret = close_ret;
	// }

	close_ret = sd_card_cleanup();
	if (close_ret != 0) {
		ret = close_ret;
	}

	printk("SD: test %s\n", ret == 0 ? "passed" : "failed");

	return ret;
}

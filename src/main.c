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
#include "main.h"

#define SD_SPI_NODE DT_NODELABEL(spi1)

// ========== Configuration ==========
#define DEBUG 0
//====================================

static const struct device *const sd_spi = DEVICE_DT_GET(SD_SPI_NODE);
static const struct gpio_dt_spec sd_cs = GPIO_DT_SPEC_GET_BY_IDX(SD_SPI_NODE, cs_gpios, 0);

dev_status all_dev_status = 
{
	.BME280 = pending,
	.xxx_sensor = pending,
	.six_seven_sensor = pending,
	.hall_effect_sensor = pending,
	.SD_card = pending
};

/*
* static functions declarations
*/
static int onboard_peripherals_init(void);
static int external_peripherals_init(void);

int main(void)
{
	// static const char data[] = "SD card write test\r\n";
	int ret;
	int close_ret;
	
	//init the data stuct and status struct:
	sd_data_struct data = {0};

	ret = onboard_peripherals_init();
	if (ret != 0) {
		return ret;
	}

	ret = external_peripherals_init();
	if (ret != 0) {
		return ret;
	}

	//sensor reading gose here???
	data.temp = 1;
	data.pressure = 2;
	data.rpm = 3;
	data.xxx = 4;
	data.yyy = 5;
	data.ccc = 6;

	ret = add_sensor_data_to_file(&data);
	if (ret != 0) {
		all_dev_status.SD_card = pending;
	} else {
		all_dev_status.SD_card = complete;
	}

	/*
	*close the SD card at very end of the firmware
	*/
	close_ret = sd_card_cleanup();
	if (close_ret != 0) {
		ret = close_ret;
	}

	return ret;
}


/*
* static functions definitions
*/

//onboard peripherals initialization gose here, for example the SD card SPI interface
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

static int onboard_peripherals_init(void){
	int ret = 0;
	//SPI1 for sd card, cs is P1.12
	ret = sd_card_spi_init();
	if (ret != 0) {
		return ret;
	}

	return ret;
}
//external devices/sensors initialization gose here, for example the BME280 sensor and SD card
static int external_peripherals_init(void)
{
	int ret = 0;

	ret = sd_card_init();
	if (ret != 0) {
		return ret;
	}

	return ret;
}
